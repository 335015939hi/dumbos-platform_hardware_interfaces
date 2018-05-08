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

Return<uint8_t> Fastboot::getSlotCount() {
    // TODO implement
    return uint8_t{};
}

Return<void> Fastboot::setFlashingLockState(bool /* lockState */,
                                            setFlashingLockState_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::setCriticalFlashingLockState(
    bool /* lockState */, setCriticalFlashingLockState_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::flashPartition(const hidl_string& /* partitionName */,
                                      const android::hardware::hidl_vec<uint8_t>& /* image */,
                                      flashPartition_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::erasePartition(const hidl_string& /* partitionName */,
                                      erasePartition_cb /* hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::setSerialConsoleState(bool /* uartState */,
                                             setSerialConsoleState_cb /* hidl_cb */) {
    // TODO implement
    return Void();
}

Return<void> Fastboot::setLogger(
    const sp<::android::hardware::fastboot::V1_0::IFastbootLogger>& /* logger */,
    setLogger_cb /* _hidl_cb */) {
    // TODO implement
    return Void();
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
