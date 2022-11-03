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

#include <aidl/android/hardware/secure_element/BnSecureElement.h>

#include <android-base/hex.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::secure_element::BnSecureElement;
using aidl::android::hardware::secure_element::ISecureElementCallback;
using aidl::android::hardware::secure_element::LogicalChannelResponse;
using android::base::HexString;
using ndk::ScopedAStatus;

static const std::vector<uint8_t> kAndroidTestAid = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41,
                                                     0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64,
                                                     0x43, 0x54, 0x53, 0x31};

class MySecureElement : public BnSecureElement {
  public:
    ScopedAStatus closeChannel(int8_t channelNumber) override {
        LOG(INFO) << __func__ << " channel number: " << channelNumber;
        if (channelNumber == 0) {
            if (mBasicChannel) {
                mBasicChannel = false;
            } else {
                return ScopedAStatus::fromServiceSpecificError(ISecureElement::FAILED);
            }
        } else if (channelNumber == 1) {
            if (mLogicChannel) {
                mLogicChannel = false;
            } else {
                return ScopedAStatus::fromServiceSpecificError(ISecureElement::FAILED);
            }
        } else {
            return ScopedAStatus::fromServiceSpecificError(ISecureElement::CHANNEL_NOT_AVAILABLE);
        }
        return ScopedAStatus::ok();
    }
    ScopedAStatus getAtr(std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__;
        _aidl_return->clear();
        return ScopedAStatus::ok();
    }
    ScopedAStatus init(const std::shared_ptr<ISecureElementCallback>& clientCallback) override {
        LOG(INFO) << __func__ << " callback: " << clientCallback.get();
        if (!clientCallback) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        mCb = clientCallback;
        mCb->onStateChange(true, "");
        return ScopedAStatus::ok();
    }
    ScopedAStatus isCardPresent(bool* _aidl_return) override {
        LOG(INFO) << __func__;
        *_aidl_return = true;
        return ScopedAStatus::ok();
    }
    ScopedAStatus openBasicChannel(const std::vector<uint8_t>& aid, int8_t p2,
                                   std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " p2 " << p2;
        if (mBasicChannel) {
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }
        mBasicChannel = true;
        *_aidl_return = {0x90, 0x00, 0x00};
        return ScopedAStatus::ok();
    }
    ScopedAStatus openLogicalChannel(
            const std::vector<uint8_t>& aid, int8_t p2,
            ::aidl::android::hardware::secure_element::LogicalChannelResponse* _aidl_return)
            override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " p2 " << p2;
        if (aid != kAndroidTestAid) {
            return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
        }
        if (mLogicChannel) {
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }
        *_aidl_return = LogicalChannelResponse{.channelNumber = 1, .selectResponse = {0x42, 0x42}};
        mLogicChannel = true;
        return ScopedAStatus::ok();
    }
    ScopedAStatus reset() override {
        LOG(INFO) << __func__;
        if (!mCb) return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        mCb->onStateChange(false, "reset");
        mCb->onStateChange(true, "reset");
        return ScopedAStatus::ok();
    }
    ScopedAStatus transmit(const std::vector<uint8_t>& data,
                           std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__ << " data: " << HexString(data.data(), data.size());
        *_aidl_return = {0x90, 0x00, 0x00};
        return ScopedAStatus::ok();
    }

  private:
    std::shared_ptr<ISecureElementCallback> mCb;
    bool mBasicChannel = false;
    bool mLogicChannel = false;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    auto se = ndk::SharedRefBase::make<MySecureElement>();
    const std::string name = std::string() + BnSecureElement::descriptor + "/eSE1";
    binder_status_t status = AServiceManager_addService(se->asBinder().get(), name.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
