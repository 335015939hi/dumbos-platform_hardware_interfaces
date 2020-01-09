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

#define LOG_TAG "IdentityCredentialHidlHalTest"

#include <map>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/identity/1.0/IIdentityCredentialStore.h>
#include <android/hardware/identity/1.0/types.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>
#include <cppbor_parse.h>

using ::testing::VtsHalHidlTargetTestEnvBase;

using std::map;
using std::string;
using std::vector;

namespace android {
namespace hardware {
namespace identity {
namespace V1_0 {
namespace test {

// ---------------------------------------------------------------------------
// Test Data.
// ---------------------------------------------------------------------------

struct TestEntryData {
    TestEntryData(string nameSpace, string name, vector<uint16_t> profileIds)
        : nameSpace(nameSpace), name(name), profileIds(profileIds) {}

    TestEntryData(string nameSpace, string name, const string& value, vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Tstr(((const char*)value.data())).encode();
    }
    TestEntryData(string nameSpace, string name, const vector<uint8_t>& value,
                  vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bstr(value).encode();
    }
    TestEntryData(string nameSpace, string name, bool value, vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bool(value).encode();
    }
    TestEntryData(string nameSpace, string name, int64_t value, vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        if (value >= 0) {
            valueCbor = cppbor::Uint(value).encode();
        } else {
            valueCbor = cppbor::Nint(-value).encode();
        }
    }

    string nameSpace;
    string name;
    vector<uint8_t> valueCbor;
    vector<uint16_t> profileIds;
};

struct TestProfile {
    uint16_t id;
    hidl_vec<uint8_t> readerCertificate;
    bool userAuthenticationRequired;
    uint64_t timeout;
};

/************************************
 *   TEST DATA FOR AUTHENTICATION
 ************************************/
// Test authentication token for user authentication

class IdentityCredentialStoreHidlEnvironment : public VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static IdentityCredentialStoreHidlEnvironment* Instance() {
        static IdentityCredentialStoreHidlEnvironment* instance =
                new IdentityCredentialStoreHidlEnvironment;
        return instance;
    }
    virtual void registerTestServices() override {
        registerTestService<IIdentityCredentialStore>();
    }

  private:
    IdentityCredentialStoreHidlEnvironment() {}
};

// The main test class credential HIDL HAL.
class IdentityCredentialStoreHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
        string serviceName = IdentityCredentialStoreHidlEnvironment::Instance()
                                     ->getServiceName<IIdentityCredentialStore>();
        ASSERT_FALSE(serviceName.empty());
        credentialStore_ =
                ::testing::VtsHalHidlTargetTestBase::getService<IIdentityCredentialStore>(
                        serviceName);
        ASSERT_NE(credentialStore_, nullptr);

        credentialStore_->getHardwareInformation(
                [&](const Result& result, const hidl_string& credentialStoreName,
                    const hidl_string& credentialStoreAuthorName, uint32_t chunkSize,
                    bool /* isDirectAccess */,
                    const hidl_vec<hidl_string> /* supportedDocTypes */) {
                    EXPECT_EQ("", result.message);
                    ASSERT_EQ(ResultCode::OK, result.code);
                    ASSERT_GT(credentialStoreName.size(), 0u);
                    ASSERT_GT(credentialStoreAuthorName.size(), 0u);
                    ASSERT_GE(chunkSize, 256u);  // Chunk sizes < APDU buffer won't be supported
                    dataChunkSize_ = chunkSize;
                });
    }

    virtual void TearDown() override {}

    bool SetupWritableCredential(sp<IWritableIdentityCredential>& writableCredential) {
        string docType = "org.iso.18013-5.2019.mdl";
        bool testCredential = true;
        Result result;
        credentialStore_->createCredential(
                docType, testCredential,
                [&](const Result& _result,
                    const sp<IWritableIdentityCredential>& _writableCredential) {
                    result = _result;
                    writableCredential = _writableCredential;
                });

        if (result.message == "" && ResultCode::OK == result.code &&
            writableCredential != nullptr) {
            return true;
        } else {
            return false;
        }
    }

    bool GenerateReaderCertificate(string serialDecimal, vector<uint8_t>& readerCertificate) {
        vector<uint8_t> readerKeyPKCS8;
        vector<uint8_t> readerPublicKey;
        vector<uint8_t> readerKey;
        if (!support::createEcKeyPair(readerKeyPKCS8) ||
            !support::ecKeyPairGetPublicKey(readerKeyPKCS8, readerPublicKey) ||
            !support::ecKeyPairGetPrivateKey(readerKeyPKCS8, readerKey))
            return false;

        string issuer = "Android Open Source Project";
        string subject = "Android IdentityCredential VTS Test";
        time_t validityNotBefore = time(nullptr);
        time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;
        return support::ecPublicKeyGenerateCertificate(readerPublicKey, readerKey, serialDecimal,
                                                       issuer, subject, validityNotBefore,
                                                       validityNotAfter, readerCertificate);
    }

    bool AddAccessControlProfiles(sp<IWritableIdentityCredential>& writableCredential,
                                  const vector<TestProfile>& testProfiles) {
        Result result;

        for (const auto& testProfile : testProfiles) {
            SecureAccessControlProfile profile;
            writableCredential->addAccessControlProfile(
                    testProfile.id, testProfile.readerCertificate,
                    testProfile.userAuthenticationRequired, testProfile.timeout,
                    [&](const Result& _result, const SecureAccessControlProfile& _profile) {
                        result = _result;
                        profile = _profile;
                    });

            if ("" != result.message || ResultCode::OK != result.code ||
                testProfile.id != profile.id ||
                testProfile.readerCertificate != profile.readerCertificate ||
                testProfile.userAuthenticationRequired != profile.userAuthenticationRequired ||
                testProfile.timeout != profile.timeout ||
                support::kAesGcmTagSize + support::kAesGcmIvSize != profile.mac.size()) {
                return false;
            }
        }
        return true;
    }

    bool AddEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
                  map<const TestEntryData*, vector<vector<uint8_t>>>& encryptedBlobs) {
        Result result;
        vector<vector<uint8_t>> chunks = support::chunkVector(entry.valueCbor, dataChunkSize_);

        writableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                          entry.valueCbor.size(),
                                          [&](const Result& _result) { result = _result; });

        if ("" != result.message || ResultCode::OK != result.code) {
            return false;
        }

        vector<vector<uint8_t>> encryptedChunks;
        bool returnCode = true;
        for (const auto& chunk : chunks) {
            writableCredential->addEntryValue(
                    chunk, [&](const Result& result, hidl_vec<uint8_t> encryptedContent) {
                        if ("" != result.message || ResultCode::OK != result.code ||
                            encryptedContent.size() > 0u) {
                            returnCode = false;
                        }

                        encryptedChunks.push_back(encryptedContent);
                    });
        }

        encryptedBlobs[&entry] = encryptedChunks;
        return returnCode;
    }

    uint32_t dataChunkSize_ = 0;

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_F(IdentityCredentialStoreHidlTest, HardwareConfiguration) {
    credentialStore_->getHardwareInformation(
            [&](const Result& result, const hidl_string& credentialStoreName,
                const hidl_string& credentialStoreAuthorName, uint32_t chunkSize,
                bool /* isDirectAccess */, const hidl_vec<hidl_string> /* supportedDocTypes */) {
                EXPECT_EQ("", result.message);
                ASSERT_EQ(ResultCode::OK, result.code);
                ASSERT_GT(credentialStoreName.size(), 0u);
                ASSERT_GT(credentialStoreAuthorName.size(), 0u);
                ASSERT_GE(chunkSize, 256u);  // Chunk sizes < APDU buffer won't be supported
            });
}

TEST_F(IdentityCredentialStoreHidlTest, verifyAttestationFail) {
    sp<IWritableIdentityCredential> writableCredential;
    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    Result result;
    credentialStore_->createCredential(
            docType, testCredential,
            [&](const Result& _result, const sp<IWritableIdentityCredential>& _writableCredential) {
                result = _result;
                writableCredential = _writableCredential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(writableCredential, nullptr);

    // test empty challenge should fail
    vector<uint8_t> attestationChallenge;
    vector<uint8_t> attestationCertificate;
    writableCredential->getAttestationCertificate(
            attestationChallenge,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);

    // shouldn't result fail with empty challenge????
    //
    EXPECT_EQ(ResultCode::FAILED, result.code);
    EXPECT_FALSE(support::certificateChainValidate(attestationCertificate));
}

TEST_F(IdentityCredentialStoreHidlTest, verifyAttestationSuccess) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge1NotSoRandomChallenge1NotSoRandomChallenge1";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<uint8_t> attestationCertificate;
    writableCredential->getAttestationCertificate(
            attestationChallenge,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);

    EXPECT_EQ(ResultCode::OK, result.code);
    EXPECT_TRUE(support::certificateChainValidate(attestationCertificate));
}

TEST_F(IdentityCredentialStoreHidlTest, verifyAttestationDoubleCall) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge1";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<uint8_t> attestationCertificate;
    writableCredential->getAttestationCertificate(
            attestationChallenge,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);

    // shouldn't result fail with empty challenge????
    //
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_TRUE(support::certificateChainValidate(attestationCertificate));

    string challenge2 = "NotSoRandomChallenge2";
    vector<uint8_t> attestationChallenge2(challenge2.begin(), challenge2.end());
    writableCredential->getAttestationCertificate(
            attestationChallenge2,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);

    // This should fail because getAttestationCertificate should not be allowed
    // to be called twice.
    EXPECT_EQ(ResultCode::FAILED, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifystartPersonalization) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // First call should go though
    writableCredential->startPersonalization(5, 6,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    writableCredential->startPersonalization(7, 4,
                                             [&](const Result& _result) { result = _result; });

    // Second call to startPersonalization should have
    // failed????????????????????
    EXPECT_NE("", result.message);
    EXPECT_NE(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifystartPersonalizationMin) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(1, 1,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifystartPersonalizationZero) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(0, 0,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifystartPersonalizationLarge) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(250, 250,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifyProfileNumberMismatch) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Enter mismatched entry and profile numbers
    writableCredential->startPersonalization(5, 5,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<uint8_t> readerCertificate;
    ASSERT_TRUE(GenerateReaderCertificate("12345", readerCertificate));

    const vector<TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                              {0, readerCertificate, false, 0},
                                              // check reuse of same id
                                              {2, readerCertificate, true, 1},
                                              // Profile 1 (no authentication)
                                              {4, {}, false, 0}};
    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    writableCredential->finishAddingEntries(
            [&](const Result& _result, const hidl_vec<uint8_t>& _credentialData,
                const hidl_vec<uint8_t>& _proofOfProvisioningSignature) { result = _result; });

    // finishAddingEntries should fail because the number of addAccessControlProfile mismatched with
    // startPersonalization, and beginAddEntry was not called.
    EXPECT_NE("", result.message);
    ASSERT_NE(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifyDuplicateProfileId) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Enter mismatched entry and profile numbers
    writableCredential->startPersonalization(3, 5,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<uint8_t> readerCertificate1;
    ASSERT_TRUE(GenerateReaderCertificate("123456", readerCertificate1));

    vector<uint8_t> readerCertificate2;
    ASSERT_TRUE(GenerateReaderCertificate("123456789", readerCertificate2));

    const vector<TestProfile> testProfiles = {// first profile should go though
                                              {0, readerCertificate1, false, 0},
                                              // same id, different
                                              // authentication requirement
                                              {0, readerCertificate2, true, 1},
                                              // same id, different certificate
                                              {0, {}, false, 0}};

    bool expectOk = true;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        writableCredential->addAccessControlProfile(
                testProfile.id, testProfile.readerCertificate,
                testProfile.userAuthenticationRequired, testProfile.timeout,
                [&](const Result& _result, const SecureAccessControlProfile& _profile) {
                    result = _result;
                    profile = _profile;
                });
        if (expectOk) {
            EXPECT_EQ("", result.message);
            ASSERT_EQ(ResultCode::OK, result.code);
            ASSERT_EQ(testProfile.id, profile.id);
            ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate);
            ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
            ASSERT_EQ(testProfile.timeout, profile.timeout);
            ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
            expectOk = false;
        } else {
            // should not allow duplicate id profiles.
            EXPECT_NE("", result.message);
            EXPECT_NE(ResultCode::OK, result.code);
        }
    }
}

TEST_F(IdentityCredentialStoreHidlTest, verifyInvalidReaderCertificateFails) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Enter mismatched entry and profile numbers
    writableCredential->startPersonalization(3, 5,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    // make up an invalid certificate
    vector<uint8_t> readerCertificate(3, (uint8_t)99);
    const TestProfile testProfile = {1, readerCertificate, false, 0};

    SecureAccessControlProfile profile;
    writableCredential->addAccessControlProfile(
            testProfile.id, testProfile.readerCertificate, testProfile.userAuthenticationRequired,
            testProfile.timeout,
            [&](const Result& _result, const SecureAccessControlProfile& _profile) {
                result = _result;
                profile = _profile;
            });
    EXPECT_NE("", result.message);
    ASSERT_NE(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, verifyEntryNameSpaceOrdering) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Enter mismatched entry and profile numbers
    writableCredential->startPersonalization(6, 6,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<uint8_t> readerCertificate1;
    ASSERT_TRUE(GenerateReaderCertificate("123456", readerCertificate1));

    vector<uint8_t> readerCertificate2;
    ASSERT_TRUE(GenerateReaderCertificate("123456789", readerCertificate2));

    const vector<TestProfile> testProfiles = {// first profile should go though
                                              {0, readerCertificate1, false, 0},
                                              // same id, different
                                              // authentication requirement
                                              {1, readerCertificate2, true, 1},
                                              // same id, different certificate
                                              {2, {}, false, 0}};

    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    const vector<TestEntryData> testEntries1 = {
            // test empty name space
            {"", "t name", string("Turing"), vector<uint16_t>{2}},
            {"", "Birth", string("19120623"), vector<uint16_t>{2}},
            {"PersonalData", "Last name", string("Turing"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Birth date", string("19120623"), vector<uint16_t>{0, 1}},
    };

    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(AddEntry(writableCredential, entry, encryptedBlobs));
    }
    const TestEntryData testEntry2 = {"Image", "Portrait image", string("asdfs"),
                                      vector<uint16_t>{0, 1}};

    EXPECT_TRUE(AddEntry(writableCredential, testEntry2, encryptedBlobs));

    // We expect this to fail because the namespace is out of order, all "PersonalData"
    // should have been called together
    const vector<TestEntryData> testEntries3 = {
            {"PersonalData", "First name", string("Alan"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Home address", string("Maida Vale, London, England"),
             vector<uint16_t>{0}},
    };

    for (const auto& entry : testEntries3) {
        EXPECT_FALSE(AddEntry(writableCredential, entry, encryptedBlobs));
    }

    writableCredential->finishAddingEntries(
            [&](const Result& _result, const hidl_vec<uint8_t>& _credentialData,
                const hidl_vec<uint8_t>& _proofOfProvisioningSignature) { result = _result; });

    // should fail because AddEntry should have failed earlier.
    EXPECT_NE("", result.message);
    ASSERT_NE(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialStoreHidlTest, createAndRetrieveCredential) {
    // First, generate a key-pair for the reader since its public key will be
    // part of the request data.
    vector<uint8_t> readerKeyPKCS8;
    vector<uint8_t> readerPublicKey;
    vector<uint8_t> readerKey;
    vector<uint8_t> readerCertificate;
    ASSERT_TRUE(support::createEcKeyPair(readerKeyPKCS8));
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(readerKeyPKCS8, readerPublicKey));
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(readerKeyPKCS8, readerKey));
    string serialDecimal = "1234";
    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential VTS Test";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;
    ASSERT_TRUE(support::ecPublicKeyGenerateCertificate(readerPublicKey, readerKey, serialDecimal,
                                                        issuer, subject, validityNotBefore,
                                                        validityNotAfter, readerCertificate));

    // Make the portrait image really big (just shy of 256 KiB) to ensure that
    // the chunking code gets exercised.
    vector<uint8_t> portraitImage;
    portraitImage.resize(256 * 1024 - 10);
    for (size_t n = 0; n < portraitImage.size(); n++) {
        portraitImage[n] = (uint8_t)n;
    }

    // Access control profiles:
    const vector<TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                              {0, readerCertificate, false, 0},
                                              // Profile 1 (no authentication)
                                              {1, {}, false, 0}};

    const vector<uint8_t> authToken = {};

    // Here's the actual test data:
    const vector<TestEntryData> testEntries = {
            {"PersonalData", "Last name", string("Turing"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Birth date", string("19120623"), vector<uint16_t>{0, 1}},
            {"PersonalData", "First name", string("Alan"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Home address", string("Maida Vale, London, England"),
             vector<uint16_t>{0}},
            {"Image", "Portrait image", portraitImage, vector<uint16_t>{0, 1}},
    };
    const vector<uint16_t> testEntriesEntryCounts = {static_cast<uint16_t>(testEntries.size() - 1),
                                                     1u};

    string cborPretty;
    sp<IWritableIdentityCredential> writableCredential;

    hidl_vec<uint8_t> empty{0};

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    Result result;
    credentialStore_->createCredential(
            docType, testCredential,
            [&](const Result& _result, const sp<IWritableIdentityCredential>& _writableCredential) {
                result = _result;
                writableCredential = _writableCredential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(writableCredential, nullptr);

    string challenge = "attestationChallenge";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<uint8_t> attestationCertificate;
    writableCredential->getAttestationCertificate(
            attestationChallenge,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    // TODO: check attestationCertificate:
    // - challenge is in serial
    // - is a chain of at least two entries
    // - each element in the chain is signed by the next one

    writableCredential->startPersonalization(testProfiles.size(), testEntriesEntryCounts,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<SecureAccessControlProfile> returnedSecureProfiles;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        writableCredential->addAccessControlProfile(
                testProfile.id, testProfile.readerCertificate,
                testProfile.userAuthenticationRequired, testProfile.timeout,
                [&](const Result& _result, const SecureAccessControlProfile& _profile) {
                    result = _result;
                    profile = _profile;
                });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);
        ASSERT_EQ(testProfile.id, profile.id);
        ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate);
        ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
        ASSERT_EQ(testProfile.timeout, profile.timeout);
        ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
        returnedSecureProfiles.push_back(profile);
    }

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        vector<vector<uint8_t>> chunks = support::chunkVector(entry.valueCbor, dataChunkSize_);

        writableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                          entry.valueCbor.size(),
                                          [&](const Result& _result) { result = _result; });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);

        vector<vector<uint8_t>> encryptedChunks;
        for (const auto& chunk : chunks) {
            writableCredential->addEntryValue(
                    chunk, [&](const Result& result, hidl_vec<uint8_t> encryptedContent) {
                        EXPECT_EQ("", result.message);
                        ASSERT_EQ(ResultCode::OK, result.code);
                        ASSERT_GT(encryptedContent.size(), 0u);
                        encryptedChunks.push_back(encryptedContent);
                    });
        }
        encryptedBlobs[&entry] = encryptedChunks;
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    writableCredential->finishAddingEntries(
            [&](const Result& _result, const hidl_vec<uint8_t>& _credentialData,
                const hidl_vec<uint8_t>& _proofOfProvisioningSignature) {
                result = _result;
                credentialData = _credentialData;
                proofOfProvisioningSignature = _proofOfProvisioningSignature;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    // TODO: inspect credentialData

    vector<uint8_t> proofOfProvisioning;
    ASSERT_TRUE(support::coseSignGetPayload(proofOfProvisioningSignature, proofOfProvisioning));
    ASSERT_TRUE(
            support::cborPrettyPrint(proofOfProvisioning, cborPretty, 32, {"readerCertificate"}));
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 0,\n"
            "      'readerCertificate' : <not printed>,\n"
            "    },\n"
            "    {\n"
            "      'id' : 1,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'PersonalData' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Birth date',\n"
            "        'value' : '19120623',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'First name',\n"
            "        'value' : 'Alan',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Home address',\n"
            "        'value' : 'Maida Vale, London, England',\n"
            "        'accessControlProfiles' : [0, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image' : [\n"
            "      {\n"
            "        'name' : 'Portrait image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);

    vector<uint8_t> credentialPubKey;
    ASSERT_TRUE(support::certificateChainGetTopMostKey(attestationCertificate, credentialPubKey));
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey));
    writableCredential = nullptr;

    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    sp<IIdentityCredential> credential;
    credentialStore_->getCredential(
            credentialData, CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
            [&](const Result& _result, const sp<IIdentityCredential>& _credential) {
                result = _result;
                credential = _credential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(credential, nullptr);

    vector<uint8_t> ephemeralKeyPair;
    credential->createEphemeralKeyPair(
            [&](const Result& _result, const hidl_vec<uint8_t>& _ephemeralKeyPair) {
                result = _result;
                ephemeralKeyPair = _ephemeralKeyPair;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    vector<uint8_t> ephemeralPublicKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(ephemeralKeyPair, ephemeralPublicKey));

    // Calculate requestData field and sign it with the reader key.
    vector<uint8_t> ephX, ephY;
    ASSERT_TRUE(support::ecPublicKeyGetXandY(ephemeralPublicKey, ephX, ephY));
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", ephX).add("ephY", ephY);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    cppbor::Array sessionTranscript = cppbor::Array()
                                              .add(cppbor::Semantic(24, deviceEngagementBytes))
                                              .add(cppbor::Semantic(24, eReaderPubBytes));
    vector<uint8_t> sessionTranscriptBytes = sessionTranscript.encode();

    vector<uint8_t> itemsRequestBytes =
            cppbor::Map("nameSpaces",
                        cppbor::Map()
                                .add("PersonalData", cppbor::Map()
                                                             .add("Last name", false)
                                                             .add("Birth date", false)
                                                             .add("First name", false)
                                                             .add("Home address", true))
                                .add("Image", cppbor::Map().add("Portrait image", false)))
                    .encode();
    ASSERT_TRUE(
            support::cborPrettyPrint(itemsRequestBytes, cborPretty, 32, {"EphemeralPublicKey"}));
    EXPECT_EQ(
            "{\n"
            "  'nameSpaces' : {\n"
            "    'PersonalData' : {\n"
            "      'Last name' : false,\n"
            "      'Birth date' : false,\n"
            "      'First name' : false,\n"
            "      'Home address' : true,\n"
            "    },\n"
            "    'Image' : {\n"
            "      'Portrait image' : false,\n"
            "    },\n"
            "  },\n"
            "}",
            cborPretty);
    vector<uint8_t> readerSignature;
    vector<uint8_t> dataToSign = cppbor::Array()
                                         .add("ReaderAuthentication")
                                         .add(sessionTranscript.clone())
                                         .add(cppbor::Semantic(24, itemsRequestBytes))
                                         .encode();
    ASSERT_TRUE(support::coseSignEcDsa(readerKey, {},  // content
                                       dataToSign,     // detached content
                                       readerCertificate, readerSignature));

    credential->startRetrieval(returnedSecureProfiles, authToken, itemsRequestBytes,
                               sessionTranscriptBytes, readerSignature, testEntriesEntryCounts,
                               [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    for (const auto& entry : testEntries) {
        credential->startRetrieveEntryValue(entry.nameSpace, entry.name, entry.valueCbor.size(),
                                            entry.profileIds, true,
                                            [&](const Result& _result) { result = _result; });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);

        auto it = encryptedBlobs.find(&entry);
        ASSERT_NE(it, encryptedBlobs.end());
        const vector<vector<uint8_t>>& encryptedChunks = it->second;

        vector<uint8_t> content;
        for (const auto& encryptedChunk : encryptedChunks) {
            vector<uint8_t> chunk;
            credential->retrieveEntryValue(
                    encryptedChunk, [&](const Result& _result, const hidl_vec<uint8_t>& _chunk) {
                        result = _result;
                        chunk = _chunk;
                    });
            EXPECT_EQ("", result.message);
            ASSERT_EQ(ResultCode::OK, result.code);
            content.insert(content.end(), chunk.begin(), chunk.end());
        }
        EXPECT_EQ(content, entry.valueCbor);
    }

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    vector<uint8_t> signingKeyCertificate;
    credential->generateSigningKeyPair([&](const Result& _result,
                                           const hidl_vec<uint8_t> _signingKeyBlob,
                                           const hidl_vec<uint8_t> _signingKeyCertificate) {
        result = _result;
        signingKeyBlob = _signingKeyBlob;
        signingKeyCertificate = _signingKeyCertificate;
    });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    // TODO: check signingKeyCertificate is signed by the credential key.

    vector<uint8_t> previousAuditSignatureHash = support::sha256(credentialData);
    vector<uint8_t> signature;
    vector<uint8_t> deviceNameSpacesBytes;
    credential->finishRetrieval(signingKeyBlob, previousAuditSignatureHash,
                                [&](const Result& _result, const hidl_vec<uint8_t> _signature,
                                    const hidl_vec<uint8_t> _deviceNameSpacesBytes,
                                    const AuditLogEntry& /* _auditLogEntry*/) {
                                    result = _result;
                                    signature = _signature;
                                    deviceNameSpacesBytes = _deviceNameSpacesBytes;
                                    // TODO: check auditLogEntry once we've figured out how it'll
                                    // work.
                                    //       See comment in IdentityCredential::finishRetrieval() in
                                    //       the default implementation for details.
                                });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_TRUE(support::cborPrettyPrint(deviceNameSpacesBytes, cborPretty, 32, {}));
    ASSERT_EQ(
            "{\n"
            "  'PersonalData' : {\n"
            "    'Last name' : 'Turing',\n"
            "    'Birth date' : '19120623',\n"
            "    'First name' : 'Alan',\n"
            "    'Home address' : 'Maida Vale, London, England',\n"
            "  },\n"
            "  'Image' : {\n"
            "    'Portrait image' : <bstr size=262134 "
            "sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "  },\n"
            "}",
            cborPretty);
    // The data that is signed is ["DeviceAuthentication", sessionTranscriptBytes, docType,
    // deviceNameSpacesBytes] so build up that structure
    cppbor::Array deviceAuthentication;
    deviceAuthentication.add("DeviceAuthentication");
    deviceAuthentication.add(sessionTranscript.clone());
    deviceAuthentication.add(docType);
    deviceAuthentication.add(cppbor::Semantic(24, deviceNameSpacesBytes));
    vector<uint8_t> encodedDeviceAuthentication = deviceAuthentication.encode();
    vector<uint8_t> signingPublicKey;
    EXPECT_TRUE(support::certificateChainGetTopMostKey(signingKeyCertificate, signingPublicKey));
    EXPECT_TRUE(support::coseCheckEcDsaSignature(signature,
                                                 encodedDeviceAuthentication,  // detached content
                                                 signingPublicKey));
}

}  // namespace test
}  // namespace V1_0
}  // namespace identity
}  // namespace hardware
}  // namespace android

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::android::base::InitLogging(argv, &android::base::StderrLogger);
    int status = RUN_ALL_TESTS();
    return status;
}
