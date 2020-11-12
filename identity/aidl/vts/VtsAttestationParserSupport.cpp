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

#include "VtsAttestationParserSupport.h"

#include <aidl/Gtest.h>
#include <map>

namespace android::hardware::identity::test_utils {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using ::keymaster::ASN1_OBJECT_Ptr;
using ::keymaster::AuthorizationSet;
using ::keymaster::EVP_PKEY_Ptr;
using ::keymaster::kAsn1TokenOid;
using ::keymaster::kEatTokenOid;
using ::keymaster::TAG_ATTESTATION_APPLICATION_ID;
using ::keymaster::TAG_IDENTITY_CREDENTIAL_KEY;
using ::keymaster::TAG_INCLUDE_UNIQUE_ID;
using ::keymaster::TypedTag;
using ::keymaster::X509_Ptr;

using support::certificateChainSplit;

optional<keymaster_cert_chain_t> AttestationCertificateParser::certificateChainToKeymasterChain(
        const vector<Certificate>& certificates) {
    if (certificates.size() <= 0) {
        return {};
    }

    keymaster_cert_chain_t kCert;
    kCert.entry_count = certificates.size();
    kCert.entries = (keymaster_blob_t*)malloc(sizeof(keymaster_blob_t) * kCert.entry_count);

    int index = 0;
    for (const auto& c : certificates) {
        kCert.entries[index].data_length = c.encodedCertificate.size();
        uint8_t* data = (uint8_t*)malloc(c.encodedCertificate.size());

        memcpy(data, c.encodedCertificate.data(), c.encodedCertificate.size());
        kCert.entries[index].data = (const uint8_t*)data;
        index++;
    }

    return kCert;
}

bool AttestationCertificateParser::parse() {
    optional<keymaster_cert_chain_t> cert_chain = certificateChainToKeymasterChain(origCertChain_);
    if (!cert_chain) {
        return false;
    }

    if (cert_chain.value().entry_count < 3) {
        return false;
    }

    if (!verifyChain(cert_chain.value())) {
        return false;
    }

    if (!verifyAttestationRecord(cert_chain.value().entries[0])) {
        return false;
    }

    keymaster_free_cert_chain(&cert_chain.value());
    return true;
}

ASN1_OCTET_STRING* AttestationCertificateParser::getAttestationRecord(X509* certificate,
                                                                      const char* oid_name) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(oid_name, 1));
    if (!oid.get()) return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1);
    if (location == -1) return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    if (!attest_rec_ext) return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    return attest_rec;
}

X509* AttestationCertificateParser::parseCertBlob(const keymaster_blob_t& blob) {
    const uint8_t* p = blob.data;
    return d2i_X509(nullptr, &p, blob.data_length);
}

bool AttestationCertificateParser::verifyAttestationRecord(
        const keymaster_blob_t& attestation_cert) {
    X509_Ptr cert(parseCertBlob(attestation_cert));
    if (!cert.get()) {
        return false;
    }

    ASN1_OCTET_STRING* asn1_ext = getAttestationRecord(cert.get(), kAsn1TokenOid);
    ASN1_OCTET_STRING* eat_ext = getAttestationRecord(cert.get(), kEatTokenOid);
    // Ensure that exactly one attestation extension is present.
    if ((asn1_ext == nullptr) ^ (eat_ext == nullptr)) {
        return false;
    }

    keymaster_blob_t att_unique_id = {};
    keymaster_blob_t att_challenge;
    if (asn1_ext) {
        if (parse_attestation_record(asn1_ext->data, asn1_ext->length, &att_attestation_version_,
                                     &att_attestation_security_level_, &att_keymaster_version_,
                                     &att_keymaster_security_level_, &att_challenge,
                                     &att_sw_enforced_, &att_hw_enforced_, &att_unique_id)) {
            return false;
        }
    } else {
        keymaster_blob_t verified_boot_key = {};
        keymaster_verified_boot_t verified_boot_state;
        bool device_locked;
        std::vector<int64_t> unexpected_claims;
        if (parse_eat_record(eat_ext->data, eat_ext->length, &att_attestation_version_,
                             &att_attestation_security_level_, &att_keymaster_version_,
                             &att_keymaster_security_level_, &att_challenge, &att_sw_enforced_,
                             &att_hw_enforced_, &att_unique_id, &verified_boot_key,
                             &verified_boot_state, &device_locked, &unexpected_claims)) {
            return false;
        }
        if (unexpected_claims.size() > 0) {
            return false;
        }
    }

    att_challenge_.assign(att_challenge.data, att_challenge.data + att_challenge.data_length);
    return true;
}

uint32_t AttestationCertificateParser::getKeymasterVersion() {
    return att_keymaster_version_;
}

uint32_t AttestationCertificateParser::getAttestationVersion() {
    return att_attestation_version_;
}

vector<uint8_t> AttestationCertificateParser::getAttestationChallenge() {
    return att_challenge_;
}

keymaster_security_level_t AttestationCertificateParser::getKeymasterSecurityLevel() {
    return att_keymaster_security_level_;
}

keymaster_security_level_t AttestationCertificateParser::getAttestationSecurityLevel() {
    return att_attestation_security_level_;
}

// Verify the Attestation certificates are correctly chained.
bool AttestationCertificateParser::verifyChain(const keymaster_cert_chain_t& chain) {
    for (size_t i = 0; i < chain.entry_count - 1; ++i) {
        keymaster_blob_t& key_cert_blob = chain.entries[i];
        keymaster_blob_t& signing_cert_blob = chain.entries[i + 1];

        X509_Ptr key_cert(parseCertBlob(key_cert_blob));
        X509_Ptr signing_cert(parseCertBlob(signing_cert_blob));
        if (!key_cert.get() || !signing_cert.get()) {
            return false;
        }

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        if (!signing_pubkey.get()) return false;

        if (X509_verify(key_cert.get(), signing_pubkey.get()) != 1) {
            return false;
        }

        if (i + 1 == chain.entry_count - 1) {
            // Last entry is self-signed.
            if (X509_verify(signing_cert.get(), signing_pubkey.get()) != 1) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace android::hardware::identity::test_utils
