/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.cas-DescramblerImpl"

#include <aidl/android/hardware/cas/Status.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <inttypes.h>
#include <media/cas/DescramblerAPI.h>
#include <media/hardware/CryptoAPI.h>
#include <media/stagefright/foundation/AUtils.h>
#include <sys/mman.h>
#include <utils/Log.h>

#include "DescramblerImpl.h"
#include "SharedLibrary.h"
#include "TypeConvert.h"

namespace android {
namespace hardware {
namespace cas {
namespace implementation {

#define CHECK_SUBSAMPLE_DEF(type)                                                                 \
    static_assert(sizeof(SubSample) == sizeof(type::SubSample), "SubSample: size doesn't match"); \
    static_assert(offsetof(SubSample, numBytesOfClearData) ==                                     \
                          offsetof(type::SubSample, mNumBytesOfClearData),                        \
                  "SubSample: numBytesOfClearData offset doesn't match");                         \
    static_assert(offsetof(SubSample, numBytesOfEncryptedData) ==                                 \
                          offsetof(type::SubSample, mNumBytesOfEncryptedData),                    \
                  "SubSample: numBytesOfEncryptedData offset doesn't match")

CHECK_SUBSAMPLE_DEF(DescramblerPlugin);
CHECK_SUBSAMPLE_DEF(CryptoPlugin);

DescramblerImpl::DescramblerImpl(const sp<SharedLibrary>& library, DescramblerPlugin* plugin)
    : mLibrary(library), mPluginHolder(plugin) {
    ALOGV("CTOR: plugin=%p", mPluginHolder.get());
}

DescramblerImpl::~DescramblerImpl() {
    ALOGV("DTOR: plugin=%p", mPluginHolder.get());
    release();
}

::ndk::ScopedAStatus DescramblerImpl::setMediaCasSession(const std::vector<uint8_t>& in_sessionId) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(in_sessionId).string());

    std::shared_ptr<DescramblerPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->setMediaCasSession(in_sessionId));
}

::ndk::ScopedAStatus DescramblerImpl::requiresSecureDecoderComponent(const std::string& in_mime,
                                                                     bool* _aidl_return) {
    std::shared_ptr<DescramblerPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        *_aidl_return = false;
    }

    *_aidl_return = holder->requiresSecureDecoderComponent(String8(in_mime.c_str()));
    return ::ndk::ScopedAStatus::ok();
}

static inline bool validateRangeForSize(int64_t offset, int64_t length, int64_t size) {
    return isInRange<int64_t, uint64_t>(0, (uint64_t)size, offset, (uint64_t)length);
}

::ndk::ScopedAStatus DescramblerImpl::descramble(ScramblingControl scramblingControl,
                                                 const std::vector<SubSample>& subSamples,
                                                 const SharedBuffer& srcBuffer, int64_t srcOffset,
                                                 const DestinationBuffer& dstBuffer,
                                                 int64_t dstOffset, int32_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    // heapbase's size is stored in int64_t, but mapMemory's mmap will map size in
    // size_t. If size is over SIZE_MAX, mapMemory mapMemory could succeed but the
    // mapped memory's actual size will be smaller than the reported size.
    if (srcBuffer.heapbase.size > SIZE_MAX) {
        ALOGE("Invalid hidl_memory size: %" PRIu64 "", srcBuffer.heapbase.size);
        android_errorWriteLog(0x534e4554, "79376389");
        return toStatus(BAD_VALUE);
    }

    void* srcPtr =
            mmap(NULL, 2, PROT_READ, MAP_SHARED, srcBuffer.heapbase.fd.get(), srcBuffer.offset);

    // Validate if the offset and size in the SharedBuffer is consistent with the
    // mapped heapbase, since the offset and size is controlled by client.
    if (srcPtr == NULL) {
        ALOGE("Failed to map src buffer.");
        return toStatus(BAD_VALUE);
    }

    // use 64-bit here to catch bad subsample size that might be overflowing.
    uint64_t totalBytesInSubSamples = 0;
    for (size_t i = 0; i < subSamples.size(); i++) {
        totalBytesInSubSamples +=
                (uint64_t)subSamples[i].numBytesOfClearData + subSamples[i].numBytesOfEncryptedData;
    }
    // Further validate if the specified srcOffset and requested total subsample size
    // is consistent with the source shared buffer size.
    if (!validateRangeForSize(srcOffset, totalBytesInSubSamples, srcBuffer.heapbase.size)) {
        ALOGE("Invalid srcOffset and subsample size: "
              "srcOffset %" PRIu64 ", totalBytesInSubSamples %" PRIu64
              ", srcBuffer"
              "size %" PRIu64 "",
              srcOffset, totalBytesInSubSamples, srcBuffer.heapbase.size);
        android_errorWriteLog(0x534e4554, "67962232");
        return toStatus(BAD_VALUE);
    }

    void* dstPtr = NULL;
    if (dstBuffer.type == BufferType::SHARED_MEMORY) {
        // When using shared memory, src buffer is also used as dst,
        // we don't map it again here.
        dstPtr = srcPtr;

        // In this case the dst and src would be the same buffer, need to validate
        // dstOffset against the buffer size too.
        if (!validateRangeForSize(dstOffset, totalBytesInSubSamples, srcBuffer.heapbase.size)) {
            ALOGE("Invalid dstOffset and subsample size: "
                  "dstOffset %" PRIu64 ", totalBytesInSubSamples %" PRIu64
                  ", srcBuffer"
                  "size %" PRIu64 "",
                  dstOffset, totalBytesInSubSamples, srcBuffer.heapbase.size);
            android_errorWriteLog(0x534e4554, "67962232");
            return toStatus(BAD_VALUE);
        }
    } else {
        native_handle_t* handle = android::makeFromAidl(dstBuffer.secureMemory);
        dstPtr = static_cast<void*>(handle);
    }

    // Get a local copy of the shared_ptr for the plugin. Note that before
    // calling the HIDL callback, this shared_ptr must be manually reset,
    // since the client side could proceed as soon as the callback is called
    // without waiting for this method to go out of scope.
    std::shared_ptr<DescramblerPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    // Casting hidl SubSample to DescramblerPlugin::SubSample, but need
    // to ensure structs are actually idential

    *_aidl_return =
            holder->descramble(dstBuffer.type != BufferType::SHARED_MEMORY,
                               (DescramblerPlugin::ScramblingControl)scramblingControl,
                               subSamples.size(), (DescramblerPlugin::SubSample*)subSamples.data(),
                               srcPtr, srcOffset, dstPtr, dstOffset, NULL);

    holder.reset();
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus DescramblerImpl::release() {
    ALOGV("%s: plugin=%p", __FUNCTION__, mPluginHolder.get());

    std::shared_ptr<DescramblerPlugin> holder(nullptr);
    std::atomic_store(&mPluginHolder, holder);

    return ndk::ScopedAStatus::ok();
}

}  // namespace implementation
}  // namespace cas
}  // namespace hardware
}  // namespace android
