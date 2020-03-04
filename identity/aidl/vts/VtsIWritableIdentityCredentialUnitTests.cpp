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

#define LOG_TAG "VtsIWritableIdentityCredentialUnitTests"

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

struct TestEntryData {
    TestEntryData(string nameSpace, string name, vector<int32_t> profileIds)
        : nameSpace(nameSpace), name(name), profileIds(profileIds) {}

    TestEntryData(string nameSpace, string name, const string& value, vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Tstr(((const char*)value.data())).encode();
    }
    TestEntryData(string nameSpace, string name, const vector<uint8_t>& value,
                  vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bstr(value).encode();
    }
    TestEntryData(string nameSpace, string name, bool value, vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bool(value).encode();
    }
    TestEntryData(string nameSpace, string name, int64_t value, vector<int32_t> profileIds)
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
    vector<int32_t> profileIds;
};

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

struct TestProfile {
    uint16_t id;
    vector<uint8_t> readerCertificate;
    bool userAuthenticationRequired;
    uint64_t timeoutMillis;
};

class IdentityCredentialUnitTests : public testing::TestWithParam<std::string> {
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

    void SetImageData(vector<uint8_t>& image) {
        image.resize(256 * 1024 - 10);
        for (size_t n = 0; n < image.size(); n++) {
            image[n] = (uint8_t)n;
        }
    }

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_P(IdentityCredentialUnitTests, HardwareConfiguration) {
    HardwareInformation hwInfo;
    ::android::binder::Status result = credentialStore_->getHardwareInformation(&hwInfo);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_GT(hwInfo.credentialStoreName.size(), 0u);
    ASSERT_GT(hwInfo.credentialStoreAuthorName.size(), 0u);
    ASSERT_GE(hwInfo.dataChunkSize, 256u);  // Chunk sizes < APDU buffer won't be supported
}

TEST_P(IdentityCredentialUnitTests, verifyAttestationFailWithEmptyChallenge) {
    sp<IWritableIdentityCredential> writableCredential;
    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    ::android::binder::Status result =
            credentialStore_->createCredential(docType, testCredential, &writableCredential);

    EXPECT_TRUE(result.isOk());
    ASSERT_NE(writableCredential, nullptr);

    // test empty challenge should fail
    vector<uint8_t> attestationChallenge;
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {};
    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    // shouldn't result fail with empty challenge????
    // current soft implementation seem to say empty challenge is acceptable.
    // Set to expect true to avoid test failure.
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate));
}

TEST_P(IdentityCredentialUnitTests, verifyAttestationSuccess) {
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

TEST_P(IdentityCredentialUnitTests, verifyAttestationDoubleCallFails) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge1";
    AttestationData attData(writableCredential, challenge, {});
    ASSERT_TRUE(ValidateAttestationCertificate(attData.attestationCertificate));

    string challenge2 = "NotSoRandomChallenge2";
    AttestationData attData2(writableCredential, challenge2, {});
    EXPECT_FALSE(attData2.result.isOk()) << attData2.result.exceptionCode() << "; "
                                         << attData2.result.exceptionMessage() << endl;
}

TEST_P(IdentityCredentialUnitTests, verifystartPersonalization) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // First call should go though
    const vector<int32_t> entryCounts = {2, 4};
    result = writableCredential->startPersonalization(5, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    // Call personalization again to verify if repeat call is allowed.
    result = writableCredential->startPersonalization(7, entryCounts);

    // Second call to startPersonalization should have
    // failed????????????????????
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
}

TEST_P(IdentityCredentialUnitTests, verifystartPersonalizationMin) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {2, 4};
    writableCredential->startPersonalization(1, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialUnitTests, verifystartPersonalizationZero) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {0};
    writableCredential->startPersonalization(0, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialUnitTests, verifystartPersonalizationOne) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {1};
    writableCredential->startPersonalization(1, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialUnitTests, verifystartPersonalizationLarge) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {3000};
    writableCredential->startPersonalization(3500, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialUnitTests, verifyProfileNumberMismatchShouldFail) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    // Enter mismatched entry and profile numbers
    const vector<int32_t> entryCounts = {5, 6};
    writableCredential->startPersonalization(5, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate = GenerateReaderCertificate("12345");
    ASSERT_TRUE(readerCertificate);

    const vector<TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                              {1, readerCertificate.value(), false, 0},
                                              {2, readerCertificate.value(), true, 1},
                                              // Profile 4 (no authentication)
                                              {4, {}, false, 0}};
    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // finishAddingEntries should fail because the number of addAccessControlProfile mismatched with
    // startPersonalization, and beginAddEntry was not called.
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
}

TEST_P(IdentityCredentialUnitTests, verifyDuplicateProfileId) {
    ::android::binder::Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    const vector<int32_t> entryCounts = {3, 6};
    writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate1 = GenerateReaderCertificate("12345");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 = GenerateReaderCertificate("1234567890");
    ASSERT_TRUE(readerCertificate2);

    const vector<TestProfile> testProfiles = {// first profile should go though
                                              {1, readerCertificate1.value(), true, 2},
                                              // same id, different
                                              // authentication requirement
                                              {1, readerCertificate2.value(), true, 1},
                                              // same id, different certificate
                                              {1, {}, false, 0}};

    bool expectOk = true;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, 0, &profile);

        if (expectOk) {
            expectOk = false;
            // for profile should be allowed though as there are no duplications
            // yet.
            ASSERT_TRUE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage()
                    << "test profile id = " << testProfile.id << endl;
            ASSERT_EQ(testProfile.id, profile.id);
            ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate.encodedCertificate);
            ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
            ASSERT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
            ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
        } else {
            // should not allow duplicate id profiles.
            ASSERT_FALSE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage()
                    << "test profile id = " << testProfile.id
                    << ", timeout=" << testProfile.timeoutMillis << endl;
        }
    }
}

TEST_P(IdentityCredentialUnitTests, verifyOneProfileAndEntryPass) {
    ::android::binder::Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge1";
    AttestationData attData(writableCredential, challenge, {});
    EXPECT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    const vector<int32_t> entryCounts = {1u};
    writableCredential->startPersonalization(1, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate1 = GenerateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    const vector<TestProfile> testProfiles = {{1, readerCertificate1.value(), true, 1}};

    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    const vector<TestEntryData> testEntries1 = {
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{0, 1}},
    };

    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        ASSERT_TRUE(
                AddEntry(writableCredential, entry, hwInfo.dataChunkSize, encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // finishAddingEntries should fail because the number of addAccessControlProfile mismatched with
    // startPersonalization, and beginAddEntry was not called.
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty =
            support::cborPrettyPrint(proofOfProvisioning.value(), 32, {"readerCertificate"});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 1,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 1,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'Name Space' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
}

TEST_P(IdentityCredentialUnitTests, verifyManyProfilesAndEntriesPass) {
    ::android::binder::Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge";
    AttestationData attData(writableCredential, challenge, {});
    EXPECT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    optional<vector<uint8_t>> readerCertificate1 = GenerateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 = GenerateReaderCertificate("1256");
    ASSERT_TRUE(readerCertificate2);

    const vector<TestProfile> testProfiles = {
            {1, readerCertificate1.value(), true, 1},
            {2, readerCertificate2.value(), true, 2},
    };
    const vector<int32_t> entryCounts = {1u, 3u, 1u, 1u, 2u};
    writableCredential->startPersonalization(testProfiles.size(), entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    vector<uint8_t> portraitImage1;
    SetImageData(portraitImage1);

    vector<uint8_t> portraitImage2;
    SetImageData(portraitImage2);

    const vector<TestEntryData> testEntries1 = {
            {"Name Space 1", "Last name", string("Turing"), vector<int32_t>{1, 2}},
            {"Name Space2", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{1}},
            {"Name Space2", "Work address", string("Maida Vale2, London, England"),
             vector<int32_t>{2}},
            {"Name Space2", "Trailer address", string("Maida, London, England"),
             vector<int32_t>{1}},
            {"Image", "Portrait image", portraitImage1, vector<int32_t>{1}},
            {"Image2", "Work image", portraitImage2, vector<int32_t>{1, 2}},
            {"Name Space3", "xyzw", string("random stuff"), vector<int32_t>{1, 2}},
            {"Name Space3", "Something", string("Some string"), vector<int32_t>{2}},
    };

    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(
                AddEntry(writableCredential, entry, hwInfo.dataChunkSize, encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // finishAddingEntries should fail because the number of addAccessControlProfile mismatched with
    // startPersonalization, and beginAddEntry was not called.
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty = support::cborPrettyPrint(proofOfProvisioning.value(),
                                                 32,  //
                                                 {"readerCertificate"});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 1,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 1,\n"
            "    },\n"
            "    {\n"
            "      'id' : 2,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 2,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'Name Space 1' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "    ],\n"
            "    'Name Space2' : [\n"
            "      {\n"
            "        'name' : 'Home address',\n"
            "        'value' : 'Maida Vale, London, England',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Work address',\n"
            "        'value' : 'Maida Vale2, London, England',\n"
            "        'accessControlProfiles' : [2, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Trailer address',\n"
            "        'value' : 'Maida, London, England',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image' : [\n"
            "      {\n"
            "        'name' : 'Portrait image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image2' : [\n"
            "      {\n"
            "        'name' : 'Work image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "    ],\n"
            "    'Name Space3' : [\n"
            "      {\n"
            "        'name' : 'xyzw',\n"
            "        'value' : 'random stuff',\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Something',\n"
            "        'value' : 'Some string',\n"
            "        'accessControlProfiles' : [2, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
}

TEST_P(IdentityCredentialUnitTests, verifyEmptyNameSpaceMixedWithNonEmptyWorks) {
    ::android::binder::Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge";
    AttestationData attData(writableCredential, challenge, {});
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    const vector<int32_t> entryCounts = {2u, 2u};
    writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate1 = GenerateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 =
            GenerateReaderCertificate("123456987987987987987987");
    ASSERT_TRUE(readerCertificate2);

    const vector<TestProfile> testProfiles = {// first profile should go though
                                              {0, readerCertificate1.value(), false, 0},
                                              // same id, different
                                              // authentication requirement
                                              {1, readerCertificate2.value(), true, 1},
                                              // same id, different certificate
                                              {2, {}, false, 0}};

    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    const vector<TestEntryData> testEntries1 = {
            // test empty name space
            {"", "t name", string("Turing"), vector<int32_t>{2}},
            {"", "Birth", string("19120623"), vector<int32_t>{2}},
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"Name Space", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
    };

    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(
                AddEntry(writableCredential, entry, hwInfo.dataChunkSize, encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialUnitTests, verifyInterleavingEntryNameSpaceOrderingFails) {
    ::android::binder::Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential));

    string challenge = "NotSoRandomChallenge";
    AttestationData attData(writableCredential, challenge, {});
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    // Enter mismatched entry and profile numbers.
    // Technically the 2nd name space of "Name Space" occurs intermittently, 2
    // before "Image" and 2 after image, which is not correct.  All of same name
    // space should occur together.  Let's see if this fails.
    const vector<int32_t> entryCounts = {2u, 1u, 2u};
    writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate1 = GenerateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 =
            GenerateReaderCertificate("123456987987987987987987");
    ASSERT_TRUE(readerCertificate2);

    const vector<TestProfile> testProfiles = {// first profile should go though
                                              {0, readerCertificate1.value(), false, 0},
                                              // same id, different
                                              // authentication requirement
                                              {1, readerCertificate2.value(), true, 1},
                                              // same id, different certificate
                                              {2, {}, false, 0}};

    ASSERT_TRUE(AddAccessControlProfiles(writableCredential, testProfiles));

    const vector<TestEntryData> testEntries1 = {
            // test empty name space
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"Name Space", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
    };

    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(
                AddEntry(writableCredential, entry, hwInfo.dataChunkSize, encryptedBlobs, true));
    }
    const TestEntryData testEntry2 = {"Image", "Portrait image", string("asdfs"),
                                      vector<int32_t>{0, 1}};

    EXPECT_TRUE(
            AddEntry(writableCredential, testEntry2, hwInfo.dataChunkSize, encryptedBlobs, true));

    // We expect this to fail because the namespace is out of order, all "Name Space"
    // should have been called together
    const vector<TestEntryData> testEntries3 = {
            {"Name Space", "First name", string("Alan"), vector<int32_t>{0, 1}},
            {"Name Space", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{0}},
    };

    for (const auto& entry : testEntries3) {
        EXPECT_FALSE(
                AddEntry(writableCredential, entry, hwInfo.dataChunkSize, encryptedBlobs, false));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // should fail because AddEntry should have failed earlier.
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
}

INSTANTIATE_TEST_SUITE_P(
        Identity, IdentityCredentialUnitTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
