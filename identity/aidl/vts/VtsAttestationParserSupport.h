
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

#ifndef VTS_ATTESTATION_PARSER_SUPPORT_H
#define VTS_ATTESTATION_PARSER_SUPPORT_H

//#include <aidl/Gtest.h>
#include <android/hardware/identity/IIdentityCredentialStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <keymaster/authorization_set.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/contexts/soft_attestation_cert.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/km_openssl/attestation_utils.h>

#include <vector>

namespace android::hardware::identity::test_utils {

using ::std::optional;
using ::std::string;
using ::std::vector;

using ::keymaster::AuthorizationSet;

class AttestationCertificateParser {
  public:
    AttestationCertificateParser(const vector<Certificate>& cert) : originalCertificate_(cert) {}

    optional<keymaster_cert_chain_t> certificateChainToKeymasterChain(
            const vector<Certificate>& certificates);

    bool ParseCertificate();

    ASN1_OCTET_STRING* get_attestation_record(X509* certificate);

    X509* parse_cert_blob(const keymaster_blob_t& blob);

    bool verify_attestation_record(const keymaster_blob_t& attestation_cert);

    uint32_t GetIdentityCredentialVersion();
    uint32_t GetAttestationVersion();
    string GetApplicationId();
    bool IncludeUniqueId();
    bool IncludeIdentityCredentialKey();
    keymaster_blob_t GetChallenge();
    keymaster_security_level_t GetKeymasterSecurityLevel();
    keymaster_security_level_t GetAttestationSecurityLevel();
    bool verify_chain(const keymaster_cert_chain_t& chain);

  private:
    vector<Certificate> originalCertificate_;
    AuthorizationSet att_sw_enforced_;
    AuthorizationSet att_hw_enforced_;
    uint32_t att_attestation_version_;
    uint32_t att_keymaster_version_;
    keymaster_security_level_t att_attestation_security_level_;
    keymaster_security_level_t att_keymaster_security_level_;
    keymaster_blob_t att_challenge_;
};

optional<keymaster_cert_chain_t> certificateVectorToKeymasterChain(
        const vector<vector<uint8_t>>& certificate);

optional<keymaster_cert_chain_t> convertAttestationVectorToChain(
        const vector<uint8_t>& certificateChain);

}  // namespace android::hardware::identity::test_utils

#endif  // VTS_ATTESTATION_PARSER_SUPPORT_H
