/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "KeymasterHidlTestUtils.h"

namespace android {
namespace hardware {

namespace keymaster {
namespace V4_0 {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator==(const KeyCharacteristics& a, const KeyCharacteristics& b) {
    // This isn't very efficient. Oh, well.
    AuthorizationSet a_sw(a.softwareEnforced);
    AuthorizationSet b_sw(b.softwareEnforced);
    AuthorizationSet a_tee(b.hardwareEnforced);
    AuthorizationSet b_tee(b.hardwareEnforced);

    a_sw.Sort();
    b_sw.Sort();
    a_tee.Sort();
    b_tee.Sort();

    return a_sw == b_sw && a_tee == b_tee;
}

namespace test {

const string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

const string bin2hex(const hidl_vec<uint8_t>& data) {
    string retval;
    retval.reserve(data.size() * 2 + 1);
    for (uint8_t byte : data) {
        retval.push_back(nibble2hex[0x0F & (byte >> 4)]);
        retval.push_back(nibble2hex[0x0F & byte]);
    }
    return retval;
}

X509* parse_cert_blob(const hidl_vec<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return d2i_X509(nullptr, &p, blob.size());
}

bool verify_chain(const hidl_vec<hidl_vec<uint8_t>>& chain) {
    for (size_t i = 0; i < chain.size(); ++i) {
        X509_Ptr key_cert(parse_cert_blob(chain[i]));
        X509_Ptr signing_cert;
        if (i < chain.size() - 1) {
            signing_cert.reset(parse_cert_blob(chain[i + 1]));
        } else {
            signing_cert.reset(parse_cert_blob(chain[i]));
        }
        EXPECT_TRUE(!!key_cert.get() && !!signing_cert.get());
        if (!key_cert.get() || !signing_cert.get()) return false;

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        EXPECT_TRUE(!!signing_pubkey.get());
        if (!signing_pubkey.get()) return false;

        EXPECT_EQ(1, X509_verify(key_cert.get(), signing_pubkey.get()))
                << "Verification of certificate " << i << " failed "
                << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL);

        char* cert_issuer =  //
                X509_NAME_oneline(X509_get_issuer_name(key_cert.get()), nullptr, 0);
        char* signer_subj =
                X509_NAME_oneline(X509_get_subject_name(signing_cert.get()), nullptr, 0);
        EXPECT_STREQ(cert_issuer, signer_subj) << "Cert " << i << " has wrong issuer.";
        if (i == 0) {
            char* cert_sub = X509_NAME_oneline(X509_get_subject_name(key_cert.get()), nullptr, 0);
            EXPECT_STREQ("/CN=Android Keystore Key", cert_sub)
                    << "Cert " << i << " has wrong subject.";
            OPENSSL_free(cert_sub);
        }

        OPENSSL_free(cert_issuer);
        OPENSSL_free(signer_subj);

        // if (dump_Attestations) std::cout << bin2hex(chain[i]) << std::endl;
    }

    return true;
}

// Extract attestation record from cert. Returned object is still part of cert; don't free it
// separately.
ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    EXPECT_TRUE(!!oid.get());
    if (!oid.get()) return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    EXPECT_NE(-1, location) << "Attestation extension not found in certificate";
    if (location == -1) return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    EXPECT_TRUE(!!attest_rec_ext)
            << "Found attestation extension but couldn't retrieve it?  Probably a BoringSSL bug.";
    if (!attest_rec_ext) return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    EXPECT_TRUE(!!attest_rec) << "Attestation extension contained no data";
    return attest_rec;
}

bool tag_in_list(const KeyParameter& entry) {
    // Attestations don't contain everything in key authorization lists, so we need to filter
    // the key lists to produce the lists that we expect to match the attestations.
    auto tag_list = {
            Tag::INCLUDE_UNIQUE_ID,
            Tag::BLOB_USAGE_REQUIREMENTS,
            Tag::EC_CURVE,
            Tag::HARDWARE_TYPE,
    };
    return std::find(tag_list.begin(), tag_list.end(), entry.tag) != tag_list.end();
}

AuthorizationSet filter_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(set.begin(), set.end(), std::back_inserter(filtered), tag_in_list);
    return filtered;
}

std::string make_string(const uint8_t* data, size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

bool verify_attestation_record(const string& challenge, const string& app_id,
                               AuthorizationSet expected_sw_enforced,
                               AuthorizationSet expected_tee_enforced, SecurityLevel security_level,
                               const hidl_vec<uint8_t>& attestation_cert) {
    X509_Ptr cert(parse_cert_blob(attestation_cert));
    EXPECT_TRUE(!!cert.get());
    if (!cert.get()) return false;

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    EXPECT_TRUE(!!attest_rec);
    if (!attest_rec) return false;

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_tee_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymaster_version;
    SecurityLevel att_attestation_security_level;
    SecurityLevel att_keymaster_security_level;
    HidlBuf att_challenge;
    HidlBuf att_unique_id;
    HidlBuf att_app_id;

    auto error = parse_attestation_record(attest_rec->data,                 //
                                          attest_rec->length,               //
                                          &att_attestation_version,         //
                                          &att_attestation_security_level,  //
                                          &att_keymaster_version,           //
                                          &att_keymaster_security_level,    //
                                          &att_challenge,                   //
                                          &att_sw_enforced,                 //
                                          &att_tee_enforced,                //
                                          &att_unique_id);
    EXPECT_EQ(ErrorCode::OK, error);
    if (error != ErrorCode::OK) return false;

    EXPECT_TRUE(att_attestation_version == 3);

    expected_sw_enforced.push_back(TAG_ATTESTATION_APPLICATION_ID, HidlBuf(app_id));

    EXPECT_EQ(att_keymaster_version, 4U);
    EXPECT_EQ(security_level, att_keymaster_security_level);
    EXPECT_EQ(security_level, att_attestation_security_level);

    EXPECT_EQ(challenge.length(), att_challenge.size());
    EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data(), challenge.length()));

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    EXPECT_EQ(filter_tags(expected_sw_enforced), filter_tags(att_sw_enforced));

    att_tee_enforced.Sort();
    expected_tee_enforced.Sort();
    EXPECT_EQ(filter_tags(expected_tee_enforced), filter_tags(att_tee_enforced));

    return true;
}

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android