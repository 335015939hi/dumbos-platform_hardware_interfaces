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

#include <aidl/android/hardware/cas/BnDescrambler.h>
#include <media/stagefright/foundation/ABase.h>

namespace android {
struct DescramblerPlugin;

namespace hardware {
namespace cas {
namespace implementation {

using aidl::android::hardware::cas::BnDescrambler;
using aidl::android::hardware::cas::BufferType;
using aidl::android::hardware::cas::DestinationBuffer;
using aidl::android::hardware::cas::ScramblingControl;
using aidl::android::hardware::cas::SharedBuffer;
using aidl::android::hardware::cas::Status;
using aidl::android::hardware::cas::SubSample;

class SharedLibrary;

class DescramblerImpl : public BnDescrambler {
  public:
    DescramblerImpl(const sp<SharedLibrary>& library, DescramblerPlugin* plugin);
    virtual ~DescramblerImpl();

    virtual ::ndk::ScopedAStatus setMediaCasSession(
            const std::vector<uint8_t>& in_sessionId) override;

    virtual ::ndk::ScopedAStatus requiresSecureDecoderComponent(const std::string& in_mime,
                                                                bool* _aidl_return) override;

    virtual ::ndk::ScopedAStatus descramble(ScramblingControl in_scramblingControl,
                                            const std::vector<SubSample>& in_subSamples,
                                            const SharedBuffer& in_srcBuffer, int64_t in_srcOffset,
                                            const DestinationBuffer& in_dstBuffer,
                                            int64_t in_dstOffset, int32_t* _aidl_return) override;

    virtual ::ndk::ScopedAStatus release() override;

  private:
    sp<SharedLibrary> mLibrary;
    std::shared_ptr<DescramblerPlugin> mPluginHolder;

    DISALLOW_EVIL_CONSTRUCTORS(DescramblerImpl);
};

}  // namespace implementation
}  // namespace cas
}  // namespace hardware
}  // namespace android
