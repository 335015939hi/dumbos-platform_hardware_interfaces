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

#ifndef ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H
#define ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H

#include <android/hardware/fastboot/1.0/IFastboot.h>
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

struct Fastboot : public IFastboot {
    // Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
    Return<bool> getSecureStatus() override;
    Return<bool> getFlashUnlockStatus() override;
    Return<bool> getOffModeChargeSetting() override;
    Return<int32_t> getBatteryVoltage() override;
    Return<bool> getBatteryFlashStatus() override;
    Return<::android::hardware::fastboot::V1_0::SlotName> getActiveSlot() override;
    Return<uint8_t> getSlotCount() override;
    Return<void> rebootBootloader() override;
    Return<::android::hardware::fastboot::V1_0::Result> setFlashingLockState(
        bool lockState) override;
    Return<::android::hardware::fastboot::V1_0::Result> setCriticalFlashingLockState(
        bool lockState) override;
    Return<::android::hardware::fastboot::V1_0::Result> flashPartition(
        const hidl_string& partitionName) override;
    Return<::android::hardware::fastboot::V1_0::Result> erasePartition(
        const hidl_string& partitionName) override;
    Return<::android::hardware::fastboot::V1_0::Result> setSerialConsoleState(
        bool uartState) override;
    Return<void> uploadImage(uploadImage_cb _hidl_cb) override;
    Return<void> downloadImage(downloadImage_cb _hidl_cb) override;
    Return<::android::hardware::fastboot::V1_0::Result> setCallback(
        const sp<::android::hardware::fastboot::V1_0::IFastbootCallback>& callback) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H
