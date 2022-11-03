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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::secure_element::BnSecureElement;

class MySecureElement : public BnSecureElement {
  public:
    ::ndk::ScopedAStatus closeChannel(int8_t in_channelNumber) override {
        (void)in_channelNumber;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus getAtr(std::vector<uint8_t>* _aidl_return) override {
        (void)_aidl_return;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus init(const std::shared_ptr<
                              ::aidl::android::hardware::secure_element::ISecureElementHalCallback>&
                                      in_clientCallback) override {
        (void)in_clientCallback;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus isCardPresent(bool* _aidl_return) override {
        (void)_aidl_return;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus openBasicChannel(const std::vector<uint8_t>& in_aid, int8_t in_p2,
                                          std::vector<uint8_t>* _aidl_return) override {
        (void)in_aid;        // FIXME
        (void)in_p2;         // FIXME
        (void)_aidl_return;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus openLogicalChannel(
            const std::vector<uint8_t>& in_aid, int8_t in_p2,
            ::aidl::android::hardware::secure_element::LogicalChannelResponse* _aidl_return)
            override {
        (void)in_aid;        // FIXME
        (void)in_p2;         // FIXME
        (void)_aidl_return;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus reset() override { return ::ndk::ScopedAStatus::ok(); }
    ::ndk::ScopedAStatus transmit(const std::vector<uint8_t>& in_data,
                                  std::vector<uint8_t>* _aidl_return) override {
        (void)in_data;       // FIXME
        (void)_aidl_return;  // FIXME
        return ::ndk::ScopedAStatus::ok();
    }
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
