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

#include "OemLock.h"

namespace android {
namespace hardware {
namespace oemlock {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
Return<OemLockSetUnlockAllowedByCarrierStatus> OemLock::setOemUnlockAllowedByCarrier(
        bool /* allowed */, const hidl_vec<uint8_t>& /* signature*/ ) {
    return OemLockSetUnlockAllowedByCarrierStatus::FAILED;
}

Return<void> OemLock::isOemUnlockAllowedByCarrier(isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::FAILED, true);
    return Void();
}

Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool /* allowed */) {
    return OemLockStatus::FAILED;
}

Return<void> OemLock::isOemUnlockAllowedByDevice(isOemUnlockAllowedByDevice_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::FAILED, true);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace oemlock
}  // namespace hardware
}  // namespace android
