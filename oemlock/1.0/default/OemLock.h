/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_OEMLOCK_V1_0_OEMLOCK_H
#define ANDROID_HARDWARE_OEMLOCK_V1_0_OEMLOCK_H

#include <android/hardware/oemlock/1.0/IOemLock.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace oemlock {
namespace V1_0 {
namespace implementation {

using ::android::hardware::oemlock::V1_0::IOemLock;
using ::android::hardware::oemlock::V1_0::OemLockSetUnlockAllowedByCarrierStatus;
using ::android::hardware::oemlock::V1_0::OemLockStatus;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct OemLock : public IOemLock {
    // Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
    Return<OemLockSetUnlockAllowedByCarrierStatus> setOemUnlockAllowedByCarrier(
            bool allowed, const hidl_vec<uint8_t>& signature) override;
    Return<void> isOemUnlockAllowedByCarrier(isOemUnlockAllowedByCarrier_cb _hidl_cb) override;
    Return<OemLockStatus> setOemUnlockAllowedByDevice(bool allowed) override;
    Return<void> isOemUnlockAllowedByDevice(isOemUnlockAllowedByDevice_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace oemlock
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_OEMLOCK_V1_0_OEMLOCK_H
