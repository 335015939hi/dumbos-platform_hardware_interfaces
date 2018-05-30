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

#include "fastboot/Fastboot.h"

#define MAX_DOWNLOAD_SIZE_DEFAULT 0x20000000
#define BATTERY_VOLTAGE_FLASHING_THRESHOLD_DEFAULT 0

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

sp<Fastboot> Fastboot::instance_;

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
Return<bool> Fastboot::isVerifiedBootEnabled() {
    return bool{};
}

Return<bool> Fastboot::isOffModeChargeEnabled() {
    return bool{};
}

Return<int32_t> Fastboot::getBatteryVoltageFlashingThreshold() {
    return BATTERY_VOLTAGE_FLASHING_THRESHOLD_DEFAULT;
}

Return<bool> Fastboot::getFlashingLockState() {
    return bool{};
}

Return<void> Fastboot::setFlashingLockState(bool /* lockState */,
                                            setFlashingLockState_cb /* _hidl_cb */) {
    return Void();
}

Return<bool> Fastboot::getCriticalFlashingLockState() {
    return bool{};
}

Return<void> Fastboot::setCriticalFlashingLockState(
    bool /* lockState */, setCriticalFlashingLockState_cb /* _hidl_cb */) {
    return Void();
}

Return<uint64_t> Fastboot::getMaxDownloadSize() {
    return MAX_DOWNLOAD_SIZE_DEFAULT;
}

Return<void> Fastboot::getPartitions(getPartitions_cb /* _hidl_cb */) {
    return Void();
}

Return<void> Fastboot::flashPartition(const hidl_string& /* partitionName */,
                                      const hidl_handle& /* image */, bool /* isFill */,
                                      uint64_t /* offset */, flashPartition_cb /* _hidl_cb */) {
    return Void();
}

Return<void> Fastboot::erasePartition(const hidl_string& /* partitionName */,
                                      erasePartition_cb /* _hidl_cb */) {
    return Void();
}

Return<void> Fastboot::setSerialConsoleState(bool /* uartState */,
                                             setSerialConsoleState_cb /* _hidl_cb */) {
    return Void();
}

Return<void> Fastboot::setLogger(
    const sp<::android::hardware::fastboot::V1_0::IFastbootLogger>& /* logger */,
    setLogger_cb /* _hidl_cb */) {
    return Void();
}

Fastboot::Fastboot() {}

sp<IFastboot> Fastboot::getInstance() {
    static sp<Fastboot> instance_ = new Fastboot();
    return instance_;
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
