/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.keymaster5-service"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <AndroidKeymaster5Device.h>
//#include "IKeymaster5Device.h"

// using aidl::android::hardware::identity::IdentityCredentialStore;
using aidl::android::hardware::keymaster::SecurityLevel;
using aidl::android::hardware::keymaster::V5_0::AndroidKeymaster5Device;

int main() {
    // identity config 0 thread, old keymaster uses 1 ???
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<AndroidKeymaster5Device> km5 =
            ndk::SharedRefBase::make<AndroidKeymaster5Device>(SecurityLevel::SOFTWARE);

    // auto keymaster = ::keymaster::V4_0::ng::CreateKeymasterDevice(SecurityLevel::SOFTWARE);
    // auto status = keymaster->registerAsService();

    const std::string instance = std::string() + AndroidKeymaster5Device::descriptor + "/default";
    LOG(INFO) << "instance: " << instance;
    binder_status_t status = AServiceManager_addService(km5->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
