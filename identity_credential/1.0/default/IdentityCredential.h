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

#ifndef ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_IDENTITYCREDENTIAL_H
#define ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_IDENTITYCREDENTIAL_H

#include <android/hardware/identity_credential/1.0/IIdentityCredential.h>
#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::identity_credential::V1_0::AccessControlProfileId;
using ::android::hardware::identity_credential::V1_0::Result;
using ::android::hardware::identity_credential::V1_0::ResultCode;
using ::android::hardware::identity_credential::V1_0::StartRetrievalArguments;

using MapStringToVectorOfStrings = std::map<std::string, std::vector<std::string>>;

struct IdentityCredential : public IIdentityCredential {
    explicit IdentityCredential(const hidl_vec<uint8_t>& credentialData)
        : credentialData_(credentialData) {}

    // Parses and decrypts credentialData_, return false on failure. Must be
    // called right after construction.
    ResultCode initialize();

    // Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredential follow.

    Return<void> deleteCredential(deleteCredential_cb _hidl_cb) override;
    Return<void> createEphemeralKeyPair(createEphemeralKeyPair_cb _hidl_cb) override;

    Return<void> startRetrieval(const StartRetrievalArguments& args,
                                startRetrieval_cb _hidl_cb) override;
    Return<void> startRetrieveEntryValue(const hidl_string& nameSpace, const hidl_string& name,
                                         uint32_t entrySize,
                                         const hidl_vec<uint8_t>& accessControlProfileIds,
                                         startRetrieveEntryValue_cb _hidl_cb) override;
    Return<void> retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                    retrieveEntryValue_cb _hidl_cb) override;
    Return<void> finishRetrieval(const hidl_vec<uint8_t>& signingKeyBlob,
                                 const hidl_vec<uint8_t>& previousAuditSignatureHash,
                                 finishRetrieval_cb _hidl_cb) override;

    Return<void> generateSigningKeyPair(generateSigningKeyPair_cb _hidl_cb) override;
    Return<void> provisionDirectAccessSigningKeyPair(
        const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<hidl_vec<uint8_t>>& signingKeyData,
        provisionDirectAccessSigningKeyPair_cb _hidl_cb) override;
    Return<void> getDirectAccessSigningKeyPairStatus(
        getDirectAccessSigningKeyPairStatus_cb _hidl_cb) override;
    Return<void> deprovisionDirectAccessSigningKeyPair(
        const hidl_vec<uint8_t>& signingKeyBlob,
        deprovisionDirectAccessSigningKeyPair_cb _hidl_cb) override;
    Return<void> configureDirectAccessPermissions(
        const hidl_vec<hidl_string>& itemsAllowedForDirectAccess,
        configureDirectAccessPermissions_cb _hidl_cb) override;

   private:
    // Set by constructor
    std::vector<uint8_t> credentialData_;

    // Set by initialize()
    std::string docType_;
    bool testCredential_;
    std::vector<uint8_t> storageKey_;
    std::vector<uint8_t> credentialPrivKey_;

    // Set by createEphemeralKeyPair()
    std::vector<uint8_t> ephemeralPublicKey_;

    // Set at startRetrieval() time.
    std::set<AccessControlProfileId> validatedProfileIds_;
    std::vector<uint8_t> requestData_;
    std::vector<uint16_t> requestCountsRemaining_;
    MapStringToVectorOfStrings requestedNameSpacesAndNames_;
    std::string currentNameSpace_;
    std::string currentName_;
    size_t entryRemainingBytes_;
    std::vector<uint8_t> entryBStrValue_;
    std::vector<uint8_t> entryStrValue_;

    // Set at startRetrieveEntryValue() time.
    std::vector<uint8_t> entryAdditionalData_;
    support::AuthenticatedDataBuilder authenticatedDataBuilder_;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_IDENTITY_CREDENTIAL_V1_0_IDENTITYCREDENTIAL_H
