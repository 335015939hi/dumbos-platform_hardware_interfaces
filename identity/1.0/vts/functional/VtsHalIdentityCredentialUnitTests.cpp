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
class IdentityCredentialUnitTests : public ::testing::VtsHalHidlTargetTestBase {
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

TEST_F(IdentityCredentialUnitTests, HardwareConfiguration) {
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

TEST_F(IdentityCredentialUnitTests, verifyAttestationFail) {
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

TEST_F(IdentityCredentialUnitTests, verifyAttestationSuccess) {
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

TEST_F(IdentityCredentialUnitTests, verifyAttestationDoubleCall) {
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

TEST_F(IdentityCredentialUnitTests, verifystartPersonalization) {
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

TEST_F(IdentityCredentialUnitTests, verifystartPersonalizationMin) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(1, 1,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialUnitTests, verifystartPersonalizationZero) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(0, 0,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialUnitTests, verifystartPersonalizationLarge) {
    Result result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    writableCredential->startPersonalization(250, 250,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
}

TEST_F(IdentityCredentialUnitTests, verifyProfileNumberMismatch) {
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

TEST_F(IdentityCredentialUnitTests, verifyDuplicateProfileId) {
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

TEST_F(IdentityCredentialUnitTests, verifyInvalidReaderCertificateFails) {
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

TEST_F(IdentityCredentialUnitTests, verifyEntryNameSpaceOrdering) {
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
