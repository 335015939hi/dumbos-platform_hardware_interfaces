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

#define LOG_TAG "VtsIWritableVtsAttestationUnitTests"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android/hardware/identity/IIdentityCredentialStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cppbor.h>
#include <cppbor_parse.h>
#include <gtest/gtest.h>
#include <future>
#include <map>

namespace android::hardware::identity {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

// ---------------------------------------------------------------------------
// Test Data.
// ---------------------------------------------------------------------------

struct AttestationData {
    AttestationData(sp<IWritableIdentityCredential>& writableCredential, string challenge,
                    vector<uint8_t> applicationId)
        : attestationApplicationId(applicationId) {
        // ASSERT_NE(writableCredential, nullptr);

        if (!challenge.empty()) {
            attestationChallenge.assign(challenge.begin(), challenge.end());
        }

        result = writableCredential->getAttestationCertificate(
                attestationApplicationId, attestationChallenge, &attestationCertificate);
    }

    AttestationData() {}

    vector<uint8_t> attestationChallenge;
    vector<uint8_t> attestationApplicationId;
    vector<Certificate> attestationCertificate;
    ::android::binder::Status result;
};

class AttestationCertificateParser {
  public:
    AttestationCertificateparser(Certificate& cert) : originalCertificate_(cert) {}

    bool ParseCertificate() {
        optional<keymaster_cert_chain_t> cert_chain =
                convertAttestationVectorToChain(originalCertificate_);
        if (!cert_chain) {
            return false;
        }

        if ((cert_chain.entry_count != 3) && (cert_chain.entry_count != 4)) {
            return false;
        }

        if (!verify_chain(cert_chain, require_prod_root)) {
            return false;
        }

        if (!verify_attestation_record(cert_chain.entries[0])) {
            return false;
        }

        keymaster_free_cert_chain(&cert_chain.value());
        return true;
    }

    ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
        ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1));
        if (!oid.get()) return nullptr;

        int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1);
        if (location == -1) return nullptr;

        X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
        if (!attest_rec_ext) return nullptr;

        ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
        return attest_rec;
    }

    X509* parse_cert_blob(const keymaster_blob_t& blob) {
        const uint8_t* p = blob.data;
        return d2i_X509(nullptr, &p, blob.data_length);
    }

    bool verify_attestation_record(const keymaster_blob_t& attestation_cert) {
        X509_Ptr cert(parse_cert_blob(attestation_cert));
        if (!cert.get()) {
            return false;
        }

        ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
        if (!attest_rec) {
            return false;
        }

        keymaster_blob_t att_unique_id = {};
        keymaster_error_t ret = parse_attestation_record(
                attest_rec->data, attest_rec->length, &att_attestation_version,
                &att_attestation_security_level_, &att_keymaster_version_,
                &att_keymaster_security_level_, &att_challenge_, &att_sw_enforced_,
                &att_hw_enforced, &att_unique_id);
        if (ret) {
            return false;
        }

        return true;
    }

    uint32_t GetIdentityCredentialVersion() { return att_keymaster_version_; }

    string GetApplicationId() {
        keymaster_blob_t id = {nullptr, 0};
        if (!att_sw_enforced_.GetTagValue(TAG_ATTESTATION_APPLICATION_ID, &id)) {
            return "";
        }

        string idString(id.data, id.data + id.data_length);
        return idString;
    }

    bool IncludeUniqueId() {
        if (att_hw_enforced_.GetTagValue(TAG_INCLUDE_UNIQUE_ID)) {
            return true;
        }

        return false;
    }

    keymaster_blob_t GetChallenge() { return att_challenge_; }

    keymaster_security_level_t GetKeymasterSecurityLevel() { return att_keymaster_security_level_; }

    keymaster_security_level_t GetAttestationSecurityLevel() {
        return att_attestation_security_level_;
    }

    bool verify_chain(const keymaster_cert_chain_t& chain, bool require_prod_root) {
        for (size_t i = 0; i < chain.entry_count - 1; ++i) {
            keymaster_blob_t& key_cert_blob = chain.entries[i];
            keymaster_blob_t& signing_cert_blob = chain.entries[i + 1];

            X509_Ptr key_cert(parse_cert_blob(key_cert_blob));
            X509_Ptr signing_cert(parse_cert_blob(signing_cert_blob));
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

                if (require_prod_root) {
                    if (signing_cert_blob.data_length != sizeof(prod_root_key) ||
                        memcmp(signing_cert_blob.data, prod_root_key, sizeof(prod_root_key)) != 0) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

  private:
    AuthorizationSet att_sw_enforced_;
    AuthorizationSet att_hw_enforced_;
    uint32_t att_attestation_version_;
    uint32_t att_keymaster_version_;
    keymaster_security_level_t att_attestation_security_level_;
    keymaster_security_level_t att_keymaster_security_level_;
    keymaster_blob_t att_challenge_ = {};
}

class VtsAttestationUnitTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }
    bool SetupWritableCredential(sp<IWritableIdentityCredential>& writableCredential) {
        string docType = "org.iso.18013-5.2019.mdl";
        bool testCredential = true;
        ::android::binder::Status result =
                credentialStore_->createCredential(docType, testCredential, &writableCredential);

        if (result.isOk() && writableCredential != nullptr) {
            return true;
        } else {
            return false;
        }
    }

    optional<vector<uint8_t>> GenerateReaderCertificate(string serialDecimal) {
        optional<vector<uint8_t>> readerKeyPKCS8 = support::createEcKeyPair();
        if (!readerKeyPKCS8) {
            return {};
        }

        optional<vector<uint8_t>> readerPublicKey =
                support::ecKeyPairGetPublicKey(readerKeyPKCS8.value());
        optional<vector<uint8_t>> readerKey =
                support::ecKeyPairGetPrivateKey(readerKeyPKCS8.value());
        if (!readerPublicKey || !readerKey) {
            return {};
        }

        string issuer = "Android Open Source Project";
        string subject = "Android IdentityCredential VTS Test";
        time_t validityNotBefore = time(nullptr);
        time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

        return support::ecPublicKeyGenerateCertificate(readerPublicKey.value(), readerKey.value(),
                                                       serialDecimal, issuer, subject,
                                                       validityNotBefore, validityNotAfter);
    }

    bool AddAccessControlProfiles(sp<IWritableIdentityCredential>& writableCredential,
                                  const vector<TestProfile>& testProfiles) {
        ::android::binder::Status result;

        for (const auto& testProfile : testProfiles) {
            SecureAccessControlProfile profile;
            Certificate cert;
            cert.encodedCertificate = testProfile.readerCertificate;
            result = writableCredential->addAccessControlProfile(
                    testProfile.id, cert, testProfile.userAuthenticationRequired,
                    testProfile.timeoutMillis, 0, &profile);

            // Don't use assert so all errors can be outputed.  Then return
            // instead of exit even on errors so caller can decide.
            EXPECT_TRUE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage()
                    << "test profile id = " << testProfile.id << endl;
            EXPECT_EQ(testProfile.id, profile.id);
            EXPECT_EQ(testProfile.readerCertificate, profile.readerCertificate.encodedCertificate);
            EXPECT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
            EXPECT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
            EXPECT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());

            if (!result.isOk() || testProfile.id != profile.id ||
                testProfile.readerCertificate != profile.readerCertificate.encodedCertificate ||
                testProfile.userAuthenticationRequired != profile.userAuthenticationRequired ||
                testProfile.timeoutMillis != profile.timeoutMillis ||
                support::kAesGcmTagSize + support::kAesGcmIvSize != profile.mac.size()) {
                return false;
            }
        }
        return true;
    }

    // Most test expects this function to pass. So we will print out additional
    // value if failed so more debug data can be provided.
    bool AddEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
                  int dataChunkSize,
                  map<const TestEntryData*, vector<vector<uint8_t>>>& encryptedBlobs,
                  bool expectSuccess) {
        ::android::binder::Status result;
        vector<vector<uint8_t>> chunks = support::chunkVector(entry.valueCbor, dataChunkSize);

        result = writableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                                   entry.valueCbor.size());

        if (expectSuccess) {
            EXPECT_TRUE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage() << endl
                    << "entry name = " << entry.name << ", name space=" << entry.nameSpace << endl;
        }

        if (!result.isOk()) {
            return false;
        }

        vector<vector<uint8_t>> encryptedChunks;
        for (const auto& chunk : chunks) {
            vector<uint8_t> encryptedContent;
            result = writableCredential->addEntryValue(chunk, &encryptedContent);
            if (expectSuccess) {
                EXPECT_TRUE(result.isOk())
                        << result.exceptionCode() << "; " << result.exceptionMessage() << endl
                        << "entry name = " << entry.name << ", name space = " << entry.nameSpace
                        << endl;

                EXPECT_GT(encryptedContent.size(), 0u)
                        << "entry name = " << entry.name << ", name space = " << entry.nameSpace
                        << endl;
            }

            if (!result.isOk() || encryptedContent.size() <= 0u) {
                return false;
            }

            encryptedChunks.push_back(encryptedContent);
        }

        encryptedBlobs[&entry] = encryptedChunks;
        return true;
    }

    bool ValidateAttestationCertificate(vector<Certificate>& inputCertificates) {
        return (inputCertificates.size() >= 2);
        // TODO: add parsing of the certificate and make sure it is genuine.
    }

    void VerifyChallenge(keymaster_blob_t challenge) {
        keymaster_blob_t att_challenge = certParser_.GetChallenge();
        EXPECT_EQ(challenge.length(), att_challenge.data_length);
        EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data, challenge.length()));
    }

    void VerifyApplicationId(string id) {
        attId = certParser_.GetApplicationId();
        EXPECT_EQ(attId, id);
    }

    sp<IIdentityCredentialStore> credentialStore_;
    AttestationCertificateParser certParser_;
};

TEST_P(VtsAttestationUnitTests, verifyAttestationSuccess) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge1NotSoRandomChallenge1NotSoRandomChallenge1";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate));
}

INSTANTIATE_TEST_SUITE_P(
        Identity, VtsAttestationUnitTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
