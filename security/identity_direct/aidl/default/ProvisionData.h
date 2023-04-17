/*
 * Copyright 2023 The Android Open Source Project
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
#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <android-base/stringprintf.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <cppbor.h>
#include <cppbor_parse.h>

#define P256_PRIV_KEY_SIZE 32
#define P256_PUB_KEY_SIZE 65
#define AES_128_KEY_SIZE 16
#define NONCE_SIZE 12

namespace aidl::android::security::identity::direct_access {

class ProvisionData {
  public:
    ProvisionData() {}
    ~ProvisionData() {}
    ProvisionData(bool in_testCredential, std::vector<uint8_t>& in_challenge, uint32_t maxCredSize);
    void setCredentialKeys(std::vector<uint8_t>& priKey, std::vector<uint8_t>& pubKey);
    std::optional<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
    presentationPackageGenerate(uint64_t validityPeriodMillis, uint64_t currentTime);
    std::optional<std::vector<uint8_t>> getEncryptedCredData(
            const std::vector<uint8_t>& in_encryptedData, /* SigningKeyAndCredentialData */
            const std::vector<uint8_t>& in_credentialData /* CredentialData */);
    bool currentPresentationPackageSet(const std::vector<uint8_t>& in_signingCertificate,
                                       const std::vector<uint8_t>& in_encryptedData);
    std::optional<std::vector<uint8_t>> getCurrentSigningCertificate();
    void clearPresentationPackage();
    void resetUsageCount();
    uint32_t getUsageCount();

  private:
    std::vector<uint8_t> mChallenge;
    bool mTestCredential;
    uint8_t mCredentialPrivKey[P256_PRIV_KEY_SIZE];
    uint8_t mCredentialPubKey[P256_PUB_KEY_SIZE];
    uint32_t mUsageCount;
    /* mDocPresentationPackage start */
    // NOTE: only one CredentialData(Presentation Package) is allowed
    std::vector<uint8_t> mSigningKeyCertificate;
    std::vector<uint8_t> mPriSigningKey;
    std::vector<uint8_t> mCborCredentialData;
    /* mDocPresentationPackage end */
};

}  // namespace aidl::android::security::identity::direct_access