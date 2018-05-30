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

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct Fastboot : public IFastboot {
    static sp<IFastboot> getInstance();
    Fastboot();
    // Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
    Return<bool> isVerifiedBootEnabled() override;
    Return<bool> isOffModeChargeEnabled() override;
    Return<int32_t> getBatteryVoltageFlashingThreshold() override;
    Return<bool> getFlashingLockState() override;
    Return<void> setFlashingLockState(bool lockState, setFlashingLockState_cb _hidl_cb) override;
    Return<bool> getCriticalFlashingLockState() override;
    Return<void> setCriticalFlashingLockState(bool lockState,
                                              setCriticalFlashingLockState_cb _hidl_cb) override;
    Return<uint64_t> getMaxDownloadSize() override;
    Return<void> getPartitions(getPartitions_cb _hidl_cb) override;
    Return<void> flashPartition(const hidl_string& partitionName, const hidl_handle& image,
                                bool isFill, uint64_t offset, flashPartition_cb _hidl_cb) override;
    Return<void> erasePartition(const hidl_string& partitionName,
                                erasePartition_cb _hidl_cb) override;
    Return<void> setSerialConsoleState(bool uartState, setSerialConsoleState_cb _hidl_cb) override;
    Return<void> setLogger(const sp<::android::hardware::fastboot::V1_0::IFastbootLogger>& logger,
                           setLogger_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
   private:
    static sp<Fastboot> instance_;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H
