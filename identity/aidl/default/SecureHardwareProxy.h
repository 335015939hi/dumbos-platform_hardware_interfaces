/*
 * Copyright 2020, The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_IDENTITY_SECUREHARDWAREPROXY_H
#define ANDROID_HARDWARE_IDENTITY_SECUREHARDWAREPROXY_H

#include <utils/RefBase.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace android::hardware::identity {

using ::android::RefBase;
using ::std::optional;
using ::std::pair;
using ::std::string;
using ::std::vector;

// These classes are used to communicate with Secure Hardware. They mimic the
// API in libEmbeddedIC 1:1 (except for using C++ types) as each call is intended
// to be forwarded to the Secure Hardware.
//
// Instances are instantiated when a provisioning or presentation session
// starts. When the session is complete, the shutdown() method is called.
//

// The proxy used for provisioning.
//
class SecureHardwareProvisioningProxy : public RefBase {
  public:
    SecureHardwareProvisioningProxy() {}
    virtual ~SecureHardwareProvisioningProxy() {}

    virtual bool initialize(bool testCredential) = 0;

    // Returns public key certificate.
    virtual optional<vector<uint8_t>> createCredentialKey(const vector<uint8_t>& challenge,
                                                          const vector<uint8_t>& applicationId) = 0;

    virtual bool startPersonalization(int accessControlProfileCount, vector<int> entryCounts,
                                      string docType, size_t expectedProofOfProvisioningSize) = 0;

    // Returns MAC (28 bytes).
    virtual optional<vector<uint8_t>> addAccessControlProfile(int id,
                                                              vector<uint8_t> readerCertificate,
                                                              bool userAuthenticationRequired,
                                                              uint64_t timeoutMillis,
                                                              uint64_t secureUserId) = 0;

    virtual bool beginAddEntry(const vector<int>& accessControlProfileIds, string nameSpace,
                               string name, uint64_t entrySize) = 0;

    // Returns encryptedContent.
    virtual optional<vector<uint8_t>> addEntryValue(const vector<int>& accessControlProfileIds,
                                                    string nameSpace, string name,
                                                    vector<uint8_t> content) = 0;

    // Returns signatureOfToBeSigned (EIC_ECDSA_P256_SIGNATURE_SIZE bytes).
    virtual optional<vector<uint8_t>> finishAddingEntries(bool testCredential) = 0;

    // Returns encryptedCredentialKeys (80 bytes).
    virtual optional<vector<uint8_t>> finishGetCredentialData(bool testCredential, string docType) = 0;

    virtual bool shutdown() = 0;
};

// The proxy used for presentation.
//
class SecureHardwarePresentationProxy : public RefBase {
  public:
    SecureHardwarePresentationProxy() {}
    virtual ~SecureHardwarePresentationProxy() {}

    virtual bool initialize(bool testCredential, string docType,
                            vector<uint8_t> encryptedCredentialKeys) = 0;

    // Returns publicKeyCert (1st component) and signingKeyBlob (2nd component)
    virtual optional<pair<vector<uint8_t>, vector<uint8_t>>> generateSigningKeyPair(string docType,
                                                                                    time_t now) = 0;

    // Returns private key
    virtual optional<vector<uint8_t>> createEphemeralKeyPair() = 0;

    virtual bool shutdown() = 0;
};

}  // namespace android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_SECUREHARDWAREPROXY_H
