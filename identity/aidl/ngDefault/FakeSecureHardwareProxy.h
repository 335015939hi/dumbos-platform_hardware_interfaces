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

#ifndef ANDROID_HARDWARE_IDENTITY_FAKESECUREHARDWAREPROXY_H
#define ANDROID_HARDWARE_IDENTITY_FAKESECUREHARDWAREPROXY_H

#include "EmbeddedIc.h"
#include "SecureHardwareProxy.h"

namespace android::hardware::identity {

// This implementation uses libEmbeddedIC in-process.
//
class FakeSecureHardwareProvisioningProxy : public SecureHardwareProvisioningProxy {
  public:
    FakeSecureHardwareProvisioningProxy();
    virtual ~FakeSecureHardwareProvisioningProxy();

    bool initialize(bool testCredential) override;

    bool shutdown() override;

    // Returns public key certificate.
    optional<vector<uint8_t>> createCredentialKey(const vector<uint8_t>& challenge,
                                                  const vector<uint8_t>& applicationId) override;

    bool startPersonalization(int accessControlProfileCount, vector<int> entryCounts,
                              string docType, size_t expectedProofOfProvisioningSize) override;

    // Returns MAC (28 bytes).
    optional<vector<uint8_t>> addAccessControlProfile(int id, vector<uint8_t> readerCertificate,
                                                      bool userAuthenticationRequired,
                                                      uint64_t timeoutMillis,
                                                      uint64_t secureUserId) override;

    bool beginAddEntry(const vector<int>& accessControlProfileIds, string nameSpace, string name,
                       uint64_t entrySize) override;

    // Returns encryptedContent.
    optional<vector<uint8_t>> addEntryValue(const vector<int>& accessControlProfileIds,
                                            string nameSpace, string name,
                                            vector<uint8_t> content) override;

    // Returns signatureOfToBeSigned (EIC_ECDSA_P256_SIGNATURE_SIZE bytes).
    optional<vector<uint8_t>> finishAddingEntries(bool testCredential) override;

    // Returns encryptedCredentialKeys (80 bytes).
    optional<vector<uint8_t>> finishGetCredentialData(bool testCredential, string docType) override;

  protected:
    EicProvisioning ctx_;
};

// This implementation uses libEmbeddedIC in-process.
//
class FakeSecureHardwarePresentationProxy : public SecureHardwarePresentationProxy {
  public:
    FakeSecureHardwarePresentationProxy();
    virtual ~FakeSecureHardwarePresentationProxy();

    bool initialize(bool testCredential, string docType,
                    vector<uint8_t> encryptedCredentialKeys) override;

    // Returns publicKeyCert (1st component) and signingKeyBlob (2nd component)
    optional<pair<vector<uint8_t>, vector<uint8_t>>> generateSigningKeyPair(string docType,
                                                                            time_t now) override;

    // Returns private key
    optional<vector<uint8_t>> createEphemeralKeyPair() override;

    bool shutdown() override;

  protected:
    EicPresentation ctx_;
};

// Factory implementation.
//
class FakeSecureHardwareProxyFactory : public SecureHardwareProxyFactory {
  public:
    FakeSecureHardwareProxyFactory() {}
    virtual ~FakeSecureHardwareProxyFactory() {}

    sp<SecureHardwareProvisioningProxy> createProvisioningProxy() override {
        return new FakeSecureHardwareProvisioningProxy();
    }

    sp<SecureHardwarePresentationProxy> createPresentationProxy() override {
        return new FakeSecureHardwarePresentationProxy();
    }
};

}  // namespace android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_FAKESECUREHARDWAREPROXY_H
