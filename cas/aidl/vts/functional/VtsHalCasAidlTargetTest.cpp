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

#define LOG_TAG "mediacas_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/cas/BnCas.h>
#include <aidl/android/hardware/cas/BnCasListener.h>
#include <aidl/android/hardware/cas/BnDescrambler.h>
#include <aidl/android/hardware/cas/BnMediaCasService.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/MemoryDealer.h>
#include <binder/ParcelFileDescriptor.h>
#include <cutils/ashmem.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#define CLEAR_KEY_SYSTEM_ID 0xF6D8
#define INVALID_SYSTEM_ID 0
#define WAIT_TIMEOUT 3000000000

#define PROVISION_STR                                      \
    "{                                                   " \
    "  \"id\": 21140844,                                 " \
    "  \"name\": \"Test Title\",                         " \
    "  \"lowercase_organization_name\": \"Android\",     " \
    "  \"asset_key\": {                                  " \
    "  \"encryption_key\": \"nezAr3CHFrmBR9R8Tedotw==\"  " \
    "  },                                                " \
    "  \"cas_type\": 1,                                  " \
    "  \"track_types\": [ ]                              " \
    "}                                                   "

using aidl::android::hardware::cas::BnCas;
using aidl::android::hardware::cas::BnCasListener;
using aidl::android::hardware::cas::BnDescrambler;
using aidl::android::hardware::cas::BnMediaCasService;
using aidl::android::hardware::cas::BufferType;
using aidl::android::hardware::cas::CasPluginDescriptor;
using aidl::android::hardware::cas::DestinationBuffer;
using aidl::android::hardware::cas::ICas;
using aidl::android::hardware::cas::ICasListener;
using aidl::android::hardware::cas::IDescrambler;
using aidl::android::hardware::cas::IMediaCasService;
using aidl::android::hardware::cas::ScramblingControl;
using aidl::android::hardware::cas::ScramblingMode;
using aidl::android::hardware::cas::SessionIntent;
using aidl::android::hardware::cas::SharedBuffer;
using aidl::android::hardware::cas::StatusEvent;
using aidl::android::hardware::cas::SubSample;
using aidl::android::hardware::common::Ashmem;
using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;
using std::shared_ptr;

const uint8_t kEcmBinaryBuffer[] = {
        0x00, 0x00, 0x01, 0xf0, 0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x46, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x27, 0x10, 0x02, 0x00,
        0x01, 0x77, 0x01, 0x42, 0x95, 0x6c, 0x0e, 0xe3, 0x91, 0xbc, 0xfd, 0x05, 0xb1, 0x60, 0x4f,
        0x17, 0x82, 0xa4, 0x86, 0x9b, 0x23, 0x56, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x27, 0x10, 0x02, 0x00, 0x01, 0x77, 0x01, 0x42, 0x95, 0x6c, 0xd7, 0x43, 0x62, 0xf8, 0x1c,
        0x62, 0x19, 0x05, 0xc7, 0x3a, 0x42, 0xcd, 0xfd, 0xd9, 0x13, 0x48,
};

const SubSample kSubSamples[] = {{162, 0}, {0, 184}, {0, 184}};

const uint8_t kInBinaryBuffer[] = {
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x6e, 0x45, 0x21,
        0x82, 0x38, 0xf0, 0x9d, 0x7d, 0x96, 0xe6, 0x94, 0xae, 0xe2, 0x87, 0x8f, 0x04, 0x49, 0xe5,
        0xf6, 0x8c, 0x8b, 0x9a, 0x10, 0x18, 0xba, 0x94, 0xe9, 0x22, 0x31, 0x04, 0x7e, 0x60, 0x5b,
        0xc4, 0x24, 0x00, 0x90, 0x62, 0x0d, 0xdc, 0x85, 0x74, 0x75, 0x78, 0xd0, 0x14, 0x08, 0xcb,
        0x02, 0x1d, 0x7d, 0x9d, 0x34, 0xe8, 0x81, 0xb9, 0xf7, 0x09, 0x28, 0x79, 0x29, 0x8d, 0xe3,
        0x14, 0xed, 0x5f, 0xca, 0xaf, 0xf4, 0x1c, 0x49, 0x15, 0xe1, 0x80, 0x29, 0x61, 0x76, 0x80,
        0x43, 0xf8, 0x58, 0x53, 0x40, 0xd7, 0x31, 0x6d, 0x61, 0x81, 0x41, 0xe9, 0x77, 0x9f, 0x9c,
        0xe1, 0x6d, 0xf2, 0xee, 0xd9, 0xc8, 0x67, 0xd2, 0x5f, 0x48, 0x73, 0xe3, 0x5c, 0xcd, 0xa7,
        0x45, 0x58, 0xbb, 0xdd, 0x28, 0x1d, 0x68, 0xfc, 0xb4, 0xc6, 0xf6, 0x92, 0xf6, 0x30, 0x03,
        0xaa, 0xe4, 0x32, 0xf6, 0x34, 0x51, 0x4b, 0x0f, 0x8c, 0xf9, 0xac, 0x98, 0x22, 0xfb, 0x49,
        0xc8, 0xbf, 0xca, 0x8c, 0x80, 0x86, 0x5d, 0xd7, 0xa4, 0x52, 0xb1, 0xd9, 0xa6, 0x04, 0x4e,
        0xb3, 0x2d, 0x1f, 0xb8, 0x35, 0xcc, 0x45, 0x6d, 0x9c, 0x20, 0xa7, 0xa4, 0x34, 0x59, 0x72,
        0xe3, 0xae, 0xba, 0x49, 0xde, 0xd1, 0xaa, 0xee, 0x3d, 0x77, 0xfc, 0x5d, 0xc6, 0x1f, 0x9d,
        0xac, 0xc2, 0x15, 0x66, 0xb8, 0xe1, 0x54, 0x4e, 0x74, 0x93, 0xdb, 0x9a, 0x24, 0x15, 0x6e,
        0x20, 0xa3, 0x67, 0x3e, 0x5a, 0x24, 0x41, 0x5e, 0xb0, 0xe6, 0x35, 0x87, 0x1b, 0xc8, 0x7a,
        0xf9, 0x77, 0x65, 0xe0, 0x01, 0xf2, 0x4c, 0xe4, 0x2b, 0xa9, 0x64, 0x96, 0x96, 0x0b, 0x46,
        0xca, 0xea, 0x79, 0x0e, 0x78, 0xa3, 0x5f, 0x43, 0xfc, 0x47, 0x6a, 0x12, 0xfa, 0xc4, 0x33,
        0x0e, 0x88, 0x1c, 0x19, 0x3a, 0x00, 0xc3, 0x4e, 0xb5, 0xd8, 0xfa, 0x8e, 0xf1, 0xbc, 0x3d,
        0xb2, 0x7e, 0x50, 0x8d, 0x67, 0xc3, 0x6b, 0xed, 0xe2, 0xea, 0xa6, 0x1f, 0x25, 0x24, 0x7c,
        0x94, 0x74, 0x50, 0x49, 0xe3, 0xc6, 0x58, 0x2e, 0xfd, 0x28, 0xb4, 0xc6, 0x73, 0xb1, 0x53,
        0x74, 0x27, 0x94, 0x5c, 0xdf, 0x69, 0xb7, 0xa1, 0xd7, 0xf5, 0xd3, 0x8a, 0x2c, 0x2d, 0xb4,
        0x5e, 0x8a, 0x16, 0x14, 0x54, 0x64, 0x6e, 0x00, 0x6b, 0x11, 0x59, 0x8a, 0x63, 0x38, 0x80,
        0x76, 0xc3, 0xd5, 0x59, 0xf7, 0x3f, 0xd2, 0xfa, 0xa5, 0xca, 0x82, 0xff, 0x4a, 0x62, 0xf0,
        0xe3, 0x42, 0xf9, 0x3b, 0x38, 0x27, 0x8a, 0x89, 0xaa, 0x50, 0x55, 0x4b, 0x29, 0xf1, 0x46,
        0x7c, 0x75, 0xef, 0x65, 0xaf, 0x9b, 0x0d, 0x6d, 0xda, 0x25, 0x94, 0x14, 0xc1, 0x1b, 0xf0,
        0xc5, 0x4c, 0x24, 0x0e, 0x65,
};

const uint8_t kOutRefBinaryBuffer[] = {
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x20, 0x2d, 0x20,
        0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d,
        0x30, 0x20, 0x72, 0x65, 0x66, 0x3d, 0x32, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b,
        0x3d, 0x31, 0x3a, 0x30, 0x3a, 0x30, 0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73, 0x65, 0x3d,
        0x30, 0x78, 0x31, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x31, 0x20, 0x6d, 0x65, 0x3d, 0x68, 0x65,
        0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65, 0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d, 0x31,
        0x20, 0x70, 0x73, 0x79, 0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e,
        0x30, 0x30, 0x20, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d, 0x31, 0x20,
        0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x3d, 0x31, 0x36, 0x20, 0x63, 0x68, 0x72,
        0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d, 0x31, 0x20, 0x74, 0x72, 0x65, 0x6c, 0x6c, 0x69,
        0x73, 0x3d, 0x31, 0x20, 0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x71,
        0x6d, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d, 0x32, 0x31,
        0x2c, 0x31, 0x31, 0x20, 0x66, 0x61, 0x73, 0x74, 0x5f, 0x70, 0x73, 0x6b, 0x69, 0x70, 0x3d,
        0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66,
        0x73, 0x65, 0x74, 0x3d, 0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d,
        0x36, 0x30, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74, 0x68,
        0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x35, 0x20, 0x73, 0x6c, 0x69, 0x63, 0x65, 0x64, 0x5f,
        0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20,
        0x64, 0x65, 0x63, 0x69, 0x6d, 0x61, 0x74, 0x65, 0x3d, 0x31, 0x20, 0x69, 0x6e, 0x74, 0x65,
        0x72, 0x6c, 0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61, 0x79,
        0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74,
        0x72, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20,
        0x62, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68,
        0x74, 0x70, 0x3d, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35, 0x30,
        0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e, 0x3d, 0x32, 0x35, 0x20,
        0x73, 0x63, 0x65, 0x6e, 0x65,
};

class MediaCasListener : public BnCasListener {
  public:
    virtual ScopedAStatus onEvent(int32_t event, int32_t arg,
                                  const std::vector<uint8_t>& data) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mEvent = event;
        mEventArg = arg;
        mEventData = data;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onSessionEvent(const std::vector<uint8_t>& sessionId, int32_t event,
                                         int32_t arg, const std::vector<uint8_t>& data) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mSessionId = sessionId;
        mEvent = event;
        mEventArg = arg;
        mEventData = data;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onStatusUpdate(StatusEvent event, int32_t arg) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mStatusEvent = event;
        mEventArg = arg;

        mEventReceived = true;
        mMsgCondition.signal();
        return ScopedAStatus::ok();
    }

    void testEventEcho(shared_ptr<ICas>& mediaCas, int32_t& event, int32_t& eventArg,
                       std::vector<uint8_t>& eventData);

    void testSessionEventEcho(shared_ptr<ICas>& mediaCas, const std::vector<uint8_t>& sessionId,
                              int32_t& event, int32_t& eventArg, std::vector<uint8_t>& eventData);

    void testStatusUpdate(shared_ptr<ICas>& mediaCas, std::vector<uint8_t>* sessionId,
                          SessionIntent intent, ScramblingMode mode);

  private:
    int32_t mEvent = -1;
    int32_t mEventArg = -1;
    StatusEvent mStatusEvent;
    bool mEventReceived = false;
    std::vector<uint8_t> mEventData;
    std::vector<uint8_t> mSessionId;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

void MediaCasListener::testEventEcho(shared_ptr<ICas>& mediaCas, int32_t& event, int32_t& eventArg,
                                     std::vector<uint8_t>& eventData) {
    mEventReceived = false;
    auto returnStatus = mediaCas->sendEvent(event, eventArg, eventData);
    EXPECT_TRUE(returnStatus.isOk());

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }

    EXPECT_EQ(mEvent, event);
    EXPECT_EQ(mEventArg, eventArg);
    EXPECT_TRUE(mEventData == eventData);
}

void MediaCasListener::testSessionEventEcho(shared_ptr<ICas>& mediaCas,
                                            const std::vector<uint8_t>& sessionId, int32_t& event,
                                            int32_t& eventArg, std::vector<uint8_t>& eventData) {
    mEventReceived = false;
    auto returnStatus = mediaCas->sendSessionEvent(sessionId, event, eventArg, eventData);
    EXPECT_TRUE(returnStatus.isOk());

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }

    EXPECT_TRUE(mSessionId == sessionId);
    EXPECT_EQ(mEvent, event);
    EXPECT_EQ(mEventArg, eventArg);
    EXPECT_TRUE(mEventData == eventData);
}

void MediaCasListener::testStatusUpdate(shared_ptr<ICas>& mediaCas, std::vector<uint8_t>* sessionId,
                                        SessionIntent intent, ScramblingMode mode) {
    mEventReceived = false;
    auto returnVoid = mediaCas->openSession(intent, mode, sessionId);
    EXPECT_TRUE(returnVoid.isOk());

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }
    EXPECT_EQ(mStatusEvent, static_cast<StatusEvent>(intent));
    EXPECT_EQ(mEventArg, static_cast<int32_t>(mode));
}

class MediaCasAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = BnMediaCasService::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(mService, nullptr);
    }

    shared_ptr<IMediaCasService> mService = nullptr;

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    shared_ptr<ICas> mMediaCas;
    shared_ptr<IDescrambler> mDescrambler;
    shared_ptr<MediaCasListener> mCasListener;

    ::testing::AssertionResult createCasPlugin(int32_t caSystemId);
    ::testing::AssertionResult openCasSession(std::vector<uint8_t>* sessionId, SessionIntent intent,
                                              ScramblingMode mode);
    ::testing::AssertionResult descrambleTestInputBuffer(
            const shared_ptr<IDescrambler>& descrambler, ScopedAStatus* descrambleStatus,
            uint8_t* inMemory);
};

::testing::AssertionResult MediaCasAidlTest::createCasPlugin(int32_t caSystemId) {
    bool isSystemIdSupported;
    auto status = mService->isSystemIdSupported(caSystemId, &isSystemIdSupported);
    bool skipDescrambler = false;
    if (!status.isOk() || !isSystemIdSupported) {
        return ::testing::AssertionFailure();
    }
    bool isDescramblerSupported;
    status = mService->isDescramblerSupported(caSystemId, &isDescramblerSupported);
    if (!status.isOk() || !isDescramblerSupported) {
        ALOGI("Skip Descrambler test since it's not required in cas@1.2.");
        mDescrambler = nullptr;
        skipDescrambler = true;
    }

    mCasListener = ::ndk::SharedRefBase::make<MediaCasListener>();
    status = mService->createPlugin(caSystemId, mCasListener, &mMediaCas);
    if (!status.isOk()) {
        return ::testing::AssertionFailure();
    }
    if (mMediaCas == nullptr) {
        return ::testing::AssertionFailure();
    }

    if (skipDescrambler) {
        return ::testing::AssertionSuccess();
    }

    status = mService->createDescrambler(caSystemId, &mDescrambler);
    if (!status.isOk()) {
        return ::testing::AssertionFailure();
    }

    return ::testing::AssertionResult(mDescrambler != nullptr);
}

::testing::AssertionResult MediaCasAidlTest::openCasSession(std::vector<uint8_t>* sessionId,
                                                            SessionIntent intent,
                                                            ScramblingMode mode) {
    auto status = mMediaCas->openSession(intent, mode, sessionId);
    return ::testing::AssertionResult(status.isOk());
}

::testing::AssertionResult MediaCasAidlTest::descrambleTestInputBuffer(
        const shared_ptr<IDescrambler>& descrambler, ScopedAStatus* descrambleStatus,
        uint8_t* inMemory) {
    std::vector<SubSample> subSample(kSubSamples,
                                     kSubSamples + (sizeof(kSubSamples) / sizeof(SubSample)));

    int size = sizeof(kInBinaryBuffer);
    auto fd = ashmem_create_region("vts-cas", size);
    if (fd < 0) {
        ALOGE("ashmem_create_region failed");
        return ::testing::AssertionFailure();
    }

    inMemory = static_cast<uint8_t*>(mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (inMemory == MAP_FAILED) {
        ALOGE("mmap failed");
        return ::testing::AssertionFailure();
    }

    memcpy(inMemory, kInBinaryBuffer, size);

    SharedBuffer srcBuffer = {
            .heapbase.fd = ::ndk::ScopedFileDescriptor(android::base::unique_fd(fd)),
            .heapbase.size = size,
            .offset = 0};

    DestinationBuffer dstBuffer;
    dstBuffer.type = BufferType::SHARED_MEMORY;
    dstBuffer.nonsecureMemory = std::move(srcBuffer);

    int32_t outBytes;
    *descrambleStatus = descrambler->descramble(ScramblingControl::EVENKEY /*2*/, subSample,
                                                srcBuffer, 0, dstBuffer, 0, &outBytes);
    if (!descrambleStatus->isOk()) {
        ALOGI("descramble failed, status=%d, outBytes=%u, error=%s", descrambleStatus->getStatus(),
              outBytes, descrambleStatus->getDescription().c_str());
    }
    return ::testing::AssertionResult(descrambleStatus->isOk());
}

TEST_P(MediaCasAidlTest, TestClearKeyApisWithSession) {
    description("Test that valid call sequences with SessionEvent send and receive");

    ASSERT_TRUE(createCasPlugin(CLEAR_KEY_SYSTEM_ID));

    auto returnStatus = mMediaCas->provision(PROVISION_STR);
    EXPECT_TRUE(returnStatus.isOk());

    std::vector<uint8_t> pvtData;
    pvtData.resize(256);
    returnStatus = mMediaCas->setPrivateData(pvtData);
    EXPECT_TRUE(returnStatus.isOk());

    SessionIntent intent = SessionIntent::LIVE;
    ScramblingMode mode = ScramblingMode::DVB_CSA1;

    std::vector<uint8_t> sessionId;
    ASSERT_TRUE(openCasSession(&sessionId, intent, mode));
    returnStatus = mMediaCas->setSessionPrivateData(sessionId, pvtData);
    EXPECT_TRUE(returnStatus.isOk());

    std::vector<uint8_t> streamSessionId;
    ASSERT_TRUE(openCasSession(&streamSessionId, intent, mode));
    returnStatus = mMediaCas->setSessionPrivateData(streamSessionId, pvtData);
    EXPECT_TRUE(returnStatus.isOk());

    if (mDescrambler != nullptr) {
        returnStatus = mDescrambler->setMediaCasSession(sessionId);
        EXPECT_TRUE(returnStatus.isOk());

        returnStatus = mDescrambler->setMediaCasSession(streamSessionId);
        EXPECT_TRUE(returnStatus.isOk());
    }

    std::vector<uint8_t> nullPtrVector(0);
    returnStatus = mMediaCas->refreshEntitlements(3, nullPtrVector);
    EXPECT_TRUE(returnStatus.isOk());

    std::vector<uint8_t> refreshData{0, 1, 2, 3};
    returnStatus = mMediaCas->refreshEntitlements(10, refreshData);
    EXPECT_TRUE(returnStatus.isOk());

    int32_t eventID = 1;
    int32_t eventArg = 2;
    mCasListener->testEventEcho(mMediaCas, eventID, eventArg, nullPtrVector);
    mCasListener->testSessionEventEcho(mMediaCas, sessionId, eventID, eventArg, nullPtrVector);

    eventID = 3;
    eventArg = 4;
    std::vector<uint8_t> eventData{'e', 'v', 'e', 'n', 't', 'd', 'a', 't', 'a'};
    mCasListener->testEventEcho(mMediaCas, eventID, eventArg, eventData);
    mCasListener->testSessionEventEcho(mMediaCas, sessionId, eventID, eventArg, eventData);

    mCasListener->testStatusUpdate(mMediaCas, &sessionId, intent, mode);

    std::vector<uint8_t> clearKeyEmmData{'c', 'l', 'e', 'a', 'r', 'k', 'e', 'y', 'e', 'm', 'm'};
    returnStatus = mMediaCas->processEmm(clearKeyEmmData);
    EXPECT_TRUE(returnStatus.isOk());

    std::vector<uint8_t> ecm(kEcmBinaryBuffer, kEcmBinaryBuffer + sizeof(kEcmBinaryBuffer));
    returnStatus = mMediaCas->processEcm(sessionId, ecm);
    EXPECT_TRUE(returnStatus.isOk());
    returnStatus = mMediaCas->processEcm(streamSessionId, ecm);
    EXPECT_TRUE(returnStatus.isOk());

    if (mDescrambler != nullptr) {
        bool requiresSecureDecoderComponent = true;
        returnStatus = mDescrambler->requiresSecureDecoderComponent(
                "video/avc", &requiresSecureDecoderComponent);
        EXPECT_TRUE(returnStatus.isOk());
        EXPECT_FALSE(requiresSecureDecoderComponent);

        ScopedAStatus descrambleStatus = ScopedAStatus::ok();
        uint8_t* opBuffer = nullptr;

        ASSERT_TRUE(descrambleTestInputBuffer(mDescrambler, &descrambleStatus, opBuffer));
        ASSERT_TRUE(descrambleStatus.isOk());

        int compareResult =
                memcmp(static_cast<const void*>(opBuffer),
                       static_cast<const void*>(kOutRefBinaryBuffer), sizeof(kOutRefBinaryBuffer));
        EXPECT_EQ(0, compareResult);

        returnStatus = mDescrambler->release();
        EXPECT_TRUE(returnStatus.isOk());
    }

    returnStatus = mMediaCas->release();
    EXPECT_TRUE(returnStatus.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MediaCasAidlTest);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, MediaCasAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IMediaCasService::descriptor)),
        android::PrintInstanceNameToString);

// Start thread pool to receive callbacks from AIDL service.
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
