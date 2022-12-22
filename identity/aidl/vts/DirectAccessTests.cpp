/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "TestDirectAccessTests"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <aidl/android/hardware/keymaster/VerificationToken.h>
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
#include <utility>

#include "Util.h"

namespace android::hardware::identity {

using std::endl;
using std::make_pair;
using std::map;
using std::optional;
using std::pair;
using std::string;
using std::tie;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using ::android::hardware::keymaster::HardwareAuthToken;
using ::android::hardware::keymaster::VerificationToken;
using test_utils::validateAttestationCertificate;

class TestDireactAccessTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        string halInstanceName = GetParam();
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(halInstanceName.c_str()));
        ASSERT_NE(credentialStore_, nullptr);
        halApiVersion_ = credentialStore_->getInterfaceVersion();
    }

    void init();
    sp<IIdentityCredentialStore> credentialStore_;
    sp<IIdentityCredential> credential;
    int halApiVersion_;
};

void TestDireactAccessTests::init() {
    // First, generate a key-pair for the reader since its public key will be
    // part of the request data.
    vector<uint8_t> readerKey;
    optional<vector<uint8_t>> readerCertificate =
            test_utils::generateReaderCertificate("1234", &readerKey);
    ASSERT_TRUE(readerCertificate);

    // Make the portrait image really big (just shy of 256 KiB) to ensure that
    // the chunking code gets exercised.
    vector<uint8_t> portraitImage;
    test_utils::setImageData(portraitImage);

    // Access control profiles:
    const vector<test_utils::TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                                          {0, readerCertificate.value(), false, 0},
                                                          // Profile 1 (no authentication)
                                                          {1, {}, false, 0}};

    // It doesn't matter since no user auth is needed in this particular test,
    // but for good measure, clear out the tokens we pass to the HAL.
    HardwareAuthToken authToken;
    VerificationToken verificationToken;
    authToken.challenge = 0;
    authToken.userId = 0;
    authToken.authenticatorId = 0;
    authToken.authenticatorType = ::android::hardware::keymaster::HardwareAuthenticatorType::NONE;
    authToken.timestamp.milliSeconds = 0;
    authToken.mac.clear();
    verificationToken.challenge = 0;
    verificationToken.timestamp.milliSeconds = 0;
    verificationToken.securityLevel = ::android::hardware::keymaster::SecurityLevel::SOFTWARE;
    verificationToken.mac.clear();

    // Here's the actual test data:
    const vector<test_utils::TestEntryData> testEntries = {
            {"PersonalData", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"PersonalData", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
            {"PersonalData", "First name", string("Alan"), vector<int32_t>{0, 1}},
            {"PersonalData", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{0}},
            {"Image", "Portrait image", portraitImage, vector<int32_t>{0, 1}},
    };
    const vector<int32_t> testEntriesEntryCounts = {static_cast<int32_t>(testEntries.size() - 1),
                                                    1u};
    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    string cborPretty;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    true /* testCredential */));

    string challenge = "attestationChallenge";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {1} /* atteestationApplicationId */);
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    validateAttestationCertificate(attData.attestationCertificate, attData.attestationChallenge,
                                   attData.attestationApplicationId, true);

    // This is kinda of a hack but we need to give the size of
    // ProofOfProvisioning that we'll expect to receive.
    const int32_t expectedProofOfProvisioningSize = 262861 - 326 + readerCertificate.value().size();
    // OK to fail, not available in v1 HAL
    writableCredential->setExpectedProofOfProvisioningSize(expectedProofOfProvisioningSize);
    ASSERT_TRUE(
            writableCredential->startPersonalization(testProfiles.size(), testEntriesEntryCounts)
                    .isOk());

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        ASSERT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    ASSERT_TRUE(
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature)
                    .isOk());

    // Validate the proofOfProvisioning which was returned
    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    cborPretty = cppbor::prettyPrint(proofOfProvisioning.value(), 32, {"readerCertificate"});
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

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData, &credential)
                        .isOk());
    ASSERT_NE(credential, nullptr);
}

TEST_P(TestDireactAccessTests, testMultiStoreAuthData) {
    int usageCnt = 0;
    init();

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob[5];
    Certificate signingKeyCertificate;
    vector<uint8_t> proofOfDeletion;

    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(credential->generateSigningKeyPair(&signingKeyBlob[i], &signingKeyCertificate)
                            .isOk());
        optional<vector<uint8_t>> signingPubKey =
                support::certificateChainGetTopMostKey(signingKeyCertificate.encodedCertificate);
        EXPECT_TRUE(signingPubKey);
        test_utils::verifyAuthKeyCertificate(signingKeyCertificate.encodedCertificate);
        ASSERT_TRUE(credential
                            ->storeStaticAuthenticationData(
                                    signingKeyBlob[0], time(nullptr) + (365 * 24 * 60 * 60), {0, 1})
                            .isOk());
    }

    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(credential->getSigningKeyUsageCount(signingKeyBlob[i], &usageCnt).isOk());
        ASSERT_TRUE(credential->deleteStaticAuthData(signingKeyBlob[i], &proofOfDeletion).isOk());

        // delete the same
        ASSERT_FALSE(credential->deleteStaticAuthData(signingKeyBlob[i], &proofOfDeletion).isOk());
    }
}

TEST_P(TestDireactAccessTests, testStoreAuthData) {
    int usageCnt = 0;
    init();

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    vector<uint8_t> proofOfDeletion;

    ASSERT_TRUE(credential->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());
    optional<vector<uint8_t>> signingPubKey =
            support::certificateChainGetTopMostKey(signingKeyCertificate.encodedCertificate);
    EXPECT_TRUE(signingPubKey);
    test_utils::verifyAuthKeyCertificate(signingKeyCertificate.encodedCertificate);

    ASSERT_TRUE(credential
                        ->storeStaticAuthenticationData(
                                signingKeyBlob, time(nullptr) + (365 * 24 * 60 * 60), {0, 1})
                        .isOk());
    ASSERT_TRUE(credential->getSigningKeyUsageCount(signingKeyBlob, &usageCnt).isOk());
    ASSERT_TRUE(credential->deleteStaticAuthData(signingKeyBlob, &proofOfDeletion).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TestDireactAccessTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, TestDireactAccessTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);
}  // namespace android::hardware::identity
