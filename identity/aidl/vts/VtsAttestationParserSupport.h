
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
#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>
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
using ::keymaster::TypedEnumTag;
using ::keymaster::TypedTag;

class AttestationCertificateParser {
  public:
    AttestationCertificateParser(const vector<Certificate>& certChain)
        : origCertChain_(certChain) {}

    bool parse();

    int getVersion() { return x509_version_; }

    int getSerialNumber() { return x509_serial_number_; }

    int getSignatureNid() { return x509_signature_nid_; }

    time_t getNotBefore() { return x509_not_before_; }

    time_t getNotAfter() { return x509_not_after_; }

    time_t getBatchNotAfter() { return x509_batch_not_after_; }

    uint32_t getKeymasterVersion() { return att_keymaster_version_; }

    uint32_t getAttestationVersion() { return att_attestation_version_; }

    vector<uint8_t> getAttestationUniqueId() { return att_unique_id_; }

    vector<uint8_t> getAttestationChallenge() { return att_challenge_; }

    keymaster_security_level_t getKeymasterSecurityLevel() { return att_keymaster_security_level_; }

    keymaster_security_level_t getAttestationSecurityLevel() {
        return att_attestation_security_level_;
    }

    // Returns subject name output in a manner comptible with RFC 2253.
    string getSubjectName() { return subjectName_; }

    // Returns issuer name output in a manner comptible with RFC 2253.
    string getIssuerName() { return issuerName_; }

    // Returns subject name for batch attestation key cert, output in a manner
    // comptible with RFC 2253.
    string getBatchCertSubjectName() { return batchCertSubjectName_; }

    // Note that Tag::ROOT_OF_TRUST is not included in the list of authorizations
    // returned by keymaster::parse_attestation_record()... instead we use
    // keymaster::parse_root_of_trust() which checks if it's in the hw-enforced
    // list and has the correct structure.
    //
    // This method returns true if this is satisfied, false otherwise.
    //
    bool hasHwEnforcedRootOfTrust() { return hasHwEnforcedRootOfTrust_; }

    template <keymaster_tag_t Tag>
    optional<uint32_t> getHwEnforcedUint(TypedTag<KM_UINT, Tag> tag) {
        uint32_t value;
        if (!att_hw_enforced_.GetTagValue(tag, &value)) {
            return {};
        }
        return value;
    }

    template <keymaster_tag_t Tag, typename T>
    optional<T> getHwEnforcedEnum(TypedEnumTag<KM_ENUM, Tag, T> tag) {
        T value;
        if (!att_hw_enforced_.GetTagValue(tag, &value)) {
            return {};
        }
        return value;
    }

    template <keymaster_tag_t Tag, typename T>
    bool containsHwEnforcedEnumRep(TypedEnumTag<KM_ENUM_REP, Tag, T> tag, T value) {
        return att_hw_enforced_.Contains(tag, value);
    }

    template <keymaster_tag_t Tag>
    bool containsHwEnforcedBool(TypedTag<KM_BOOL, Tag> tag) {
        if (att_hw_enforced_.GetTagValue(tag)) {
            return true;
        }
        return false;
    }

    template <keymaster_tag_t Tag>
    optional<vector<uint8_t>> getHwEnforcedBlob(TypedTag<KM_BYTES, Tag> tag) {
        keymaster_blob_t blob;
        if (!att_hw_enforced_.GetTagValue(tag, &blob)) {
            return {};
        }
        vector<uint8_t> ret(blob.data, blob.data + blob.data_length);
        return ret;
    }

    template <keymaster_tag_t Tag>
    optional<vector<uint8_t>> getSwEnforcedBlob(TypedTag<KM_BYTES, Tag> tag) {
        keymaster_blob_t blob;
        if (!att_sw_enforced_.GetTagValue(tag, &blob)) {
            return {};
        }
        vector<uint8_t> ret(blob.data, blob.data + blob.data_length);
        return ret;
    }

  private:
    bool verifyChain(const keymaster_cert_chain_t& chain);

    ASN1_OCTET_STRING* getAttestationRecord(X509* certificate);

    X509* parseCertBlob(const keymaster_blob_t& blob);

    bool extractFromTopCert(const keymaster_blob_t& attestation_cert);

    bool extractFromBatchCert(const keymaster_blob_t& attestation_cert);

    optional<keymaster_cert_chain_t> certificateChainToKeymasterChain(
            const vector<Certificate>& certificates);

    vector<Certificate> origCertChain_;
    AuthorizationSet att_sw_enforced_;
    AuthorizationSet att_hw_enforced_;
    uint32_t att_attestation_version_;
    uint32_t att_keymaster_version_;
    keymaster_security_level_t att_attestation_security_level_;
    keymaster_security_level_t att_keymaster_security_level_;
    vector<uint8_t> att_challenge_;
    vector<uint8_t> att_unique_id_;
    int x509_version_;
    int x509_serial_number_;
    int x509_signature_nid_;
    time_t x509_not_before_;
    time_t x509_not_after_;
    time_t x509_batch_not_after_;

    bool hasHwEnforcedRootOfTrust_ = false;
    string subjectName_;
    string issuerName_;
    string batchCertSubjectName_;
};

}  // namespace android::hardware::identity::test_utils

#endif  // VTS_ATTESTATION_PARSER_SUPPORT_H
