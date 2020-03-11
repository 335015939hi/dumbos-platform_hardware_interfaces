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

#define LOG_TAG "VtsIWritableVtsAttestationTests"

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

#include "VtsAttestationParserSupport.h"
#include "VtsIdentityTestUtils.h"

namespace android::hardware::identity {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using test_utils::AttestationCertificateParser;
using test_utils::SetupWritableCredential;
using test_utils::ValidateAttestationCertificate;

// This file verifies the Identity Credential VTS Attestation Certificate
// generated.
class VtsAttestationTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_P(VtsAttestationTests, verifyAttestationWithEmptyChallengeEmptyId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::SetupWritableCredential(writableCredential, credentialStore_));

    vector<uint8_t> attestationChallenge;
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {};
    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_EQ(binder::Status::EX_NONE, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_OK, result.serviceSpecificErrorCode());

    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId));
}

TEST_P(VtsAttestationTests, verifyAttestationWithEmptyChallengeNonemptyId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential, credentialStore_));

    vector<uint8_t> attestationChallenge;
    vector<Certificate> attestationCertificate;
    string applicationId = "Attestation Verification";
    vector<uint8_t> attestationApplicationId = {applicationId.begin(), applicationId.end()};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_EQ(binder::Status::EX_NONE, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_OK, result.serviceSpecificErrorCode());
    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId));
}

TEST_P(VtsAttestationTests, verifyAttestationWithNonemptyChallengeEmptyId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential, credentialStore_));

    string challenge = "NotSoRandomChallenge";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_EQ(binder::Status::EX_NONE, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_OK, result.serviceSpecificErrorCode());

    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId));
}

TEST_P(VtsAttestationTests, verifyAttestationWithNonemptyChallengeNonemptyId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential, credentialStore_));

    string challenge = "NotSoRandomChallenge1NotSoRandomChallenge1NotSoRandomChallenge1";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificate;
    string applicationId = "Attestation Verification";
    vector<uint8_t> attestationApplicationId = {applicationId.begin(), applicationId.end()};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_EQ(binder::Status::EX_NONE, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_OK, result.serviceSpecificErrorCode());

    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId));
}

TEST_P(VtsAttestationTests, verifyAttestationWithVeryShortChallengeAndId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(SetupWritableCredential(writableCredential, credentialStore_));

    string challenge = "c";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificate;
    string applicationId = "i";
    vector<uint8_t> attestationApplicationId = {applicationId.begin(), applicationId.end()};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
    ASSERT_EQ(binder::Status::EX_NONE, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_OK, result.serviceSpecificErrorCode());

    EXPECT_TRUE(ValidateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId));
}

INSTANTIATE_TEST_SUITE_P(
        Identity, VtsAttestationTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
