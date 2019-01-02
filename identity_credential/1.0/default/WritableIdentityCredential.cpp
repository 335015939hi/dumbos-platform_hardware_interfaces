/*
 * Copyright 2019, The Android Open Source Project
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

#include "WritableIdentityCredential.h"

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

Return<void> WritableIdentityCredential::startPersonalization(
    const hidl_vec<uint8_t>& attestationApplicationId,
    const hidl_vec<uint8_t>& attestationChallenge, uint8_t accessControlProfileCount,
    uint16_t entryCount, startPersonalization_cb _hidl_cb) {
    return Void();
}

Return<void> WritableIdentityCredential::addAccessControlProfile(
    uint8_t id, const hidl_vec<uint8_t>& readerAuthPubKey, uint64_t capabilityId,
    CapabilityType capabilityType, uint32_t timeout, addAccessControlProfile_cb _hidl_cb) {
    return Void();
}

Return<ResultCode> WritableIdentityCredential::beginAddEntry(
    const hidl_vec<SecureAccessControlProfile>& accessControlProfiles, const hidl_string& nameSpace,
    const hidl_string& name, bool directlyAvailable, uint32_t entrySize) {
    return ResultCode::FAILED;
}

Return<void> WritableIdentityCredential::addEntryValue(
    const ::android::hardware::identity_credential::V1_0::EntryValue& value,
    addEntryValue_cb _hidl_cb) {
    return Void();
}

Return<void> WritableIdentityCredential::finishAddingEntryies(finishAddingEntryies_cb _hidl_cb) {
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
