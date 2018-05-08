/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOTLOGGER_H
#define ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOTLOGGER_H

#include <android/hardware/fastboot/1.0/IFastbootLogger.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct FastbootLogger : public IFastbootLogger {
    // Methods from ::android::hardware::fastboot::V1_0::IFastbootLogger follow.
    Return<void> onLog(const hidl_string& log) override;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IFastbootLogger* HIDL_FETCH_IFastbootLogger(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOTLOGGER_H
