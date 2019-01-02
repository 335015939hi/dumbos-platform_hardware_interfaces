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

#ifndef ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_WRITABLEIDENTITYCREDENTIAL_H
#define ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_WRITABLEIDENTITYCREDENTIAL_H

#include <android/hardware/identity_credential/1.0/IWritableIdentityCredential.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::identity_credential::V1_0::KeyType;
using ::android::hardware::identity_credential::V1_0::ResultCode;
using ::android::hardware::identity_credential::V1_0::SecureAccessControlProfile;
using ::android::hardware::identity_credential::V1_0::StartRetrievalArguments;
using ::android::hardware::keymaster::capability::V1_0::CapabilityType;

struct WritableIdentityCredential : public IWritableIdentityCredential {
    WritableIdentityCredential(const hidl_string& docType, bool testCredential)
        : doc_type_(docType), test_credential_(testCredential) {}

    // Methods from ::android::hardware::identity_credential::V1_0::IWritableIdentityCredential
    // follow.

    Return<void> startPersonalization(const hidl_vec<uint8_t>& attestationApplicationId,
                                      const hidl_vec<uint8_t>& attestationChallenge,
                                      uint8_t accessControlProfileCount, uint16_t entryCount,
                                      startPersonalization_cb _hidl_cb) override;

    Return<void> addAccessControlProfile(uint8_t id, const hidl_vec<uint8_t>& readerAuthPubKey,
                                         uint64_t capabilityId, CapabilityType capabilityType,
                                         uint32_t timeout,
                                         addAccessControlProfile_cb _hidl_cb) override;

    Return<ResultCode> beginAddEntry(
        const hidl_vec<SecureAccessControlProfile>& accessControlProfiles,
        const hidl_string& nameSpace, const hidl_string& name, bool directlyAvailable,
        uint32_t entrySize) override;

    Return<void> addEntryValue(
        const ::android::hardware::identity_credential::V1_0::EntryValue& value,
        addEntryValue_cb _hidl_cb) override;

    Return<void> finishAddingEntryies(finishAddingEntryies_cb _hidl_cb) override;

   private:
    std::string doc_type_;
    bool test_credential_;

    // These fields are initialized during startPersonalization().
    std::vector<uint8_t> storageKey_;
    size_t numAccessControlProfileRemaining_;
    size_t numEntriesRemaining_;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_WRITABLEIDENTITYCREDENTIAL_H
