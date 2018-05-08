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

#include "Fastboot.h"

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
Return<bool> Fastboot::getSecureStatus() {
    // TODO implement
    return bool{};
}

Return<bool> Fastboot::getFlashUnlockStatus() {
    // TODO implement
    return bool{};
}

Return<bool> Fastboot::getOffModeChargeSetting() {
    // TODO implement
    return bool{};
}

Return<int32_t> Fastboot::getBatteryVoltage() {
    // TODO implement
    return int32_t{};
}

Return<bool> Fastboot::getBatteryFlashStatus() {
    // TODO implement
    return bool{};
}

Return<::android::hardware::fastboot::V1_0::SlotName> Fastboot::getActiveSlot() {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::SlotName{};
}

Return<uint8_t> Fastboot::getSlotCount() {
    // TODO implement
    return uint8_t{};
}

Return<void> Fastboot::rebootBootloader() {
    // TODO implement
    return Void();
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setFlashingLockState(
    bool /* lockState */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setCriticalFlashingLockState(
    bool /* lockState */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::flashPartition(
    const hidl_string& /* partitionName */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::erasePartition(
    const hidl_string& /* partitionName */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setSerialConsoleState(
    bool /* uartState */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<void> Fastboot::uploadImage(uploadImage_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::downloadImage(downloadImage_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setCallback(
    const sp<::android::hardware::fastboot::V1_0::IFastbootCallback>& /* callback */) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IFastboot* HIDL_FETCH_IFastboot(const char* /* name */) {
    return new Fastboot();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
