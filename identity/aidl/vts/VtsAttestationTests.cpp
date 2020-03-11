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

// This file verifies the Identity Credential VTS Attestation Certificate
// generated.
class VtsAttestationTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    bool ValidateAttestationCertificate(const vector<Certificate>& inputCertificates,
                                        const vector<uint8_t>& expected_challenge,
                                        vector<uint8_t>& expected_app_id) {
        AttestationCertificateParser certParser_(inputCertificates);
        bool ret = certParser_.ParseCertificate();
        EXPECT_TRUE(ret);
        if (!ret) {
            return false;
        }

        // As per the IC HAL, the version of the Identity
        // Credential HAL is 1.0 - and this is encoded as major*10 + minor. This field is used by
        // Keymaster which is known to report integers less than or equal to 4 (for KM up to 4.0)
        // and integers greater or equal than 41 (for KM starting with 4.1).
        //
        // Since we won't get to version 4.0 of the IC HAL for a while, let's also check that a KM
        // version isn't errornously returned.
        EXPECT_LE(10, certParser_.GetIdentityCredentialVersion());
        EXPECT_GT(40, certParser_.GetIdentityCredentialVersion());
        EXPECT_LE(3, certParser_.GetAttestationVersion());

        // Verify the app id matches to whatever we set it to be.
        EXPECT_EQ(expected_app_id.size(), certParser_.GetApplicationId().size());
        EXPECT_EQ(0, memcmp(expected_app_id.data(), certParser_.GetApplicationId().data(),
                            expected_app_id.size()));

        EXPECT_FALSE(certParser_.IncludeUniqueId());
        EXPECT_TRUE(certParser_.IncludeIdentityCredentialKey());

        // Verify the challenge always matches in size and data of what is passed
        // in.
        keymaster_blob_t att_challenge = certParser_.GetChallenge();
        EXPECT_EQ(expected_challenge.size(), att_challenge.data_length);
        EXPECT_EQ(0,
                  memcmp(expected_challenge.data(), att_challenge.data, expected_challenge.size()));

        // The level is a bit tricky.  It may be different for each hardware.  May
        // need to delete this, unless there is a correlation with the version
        // number.
        EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, certParser_.GetKeymasterSecurityLevel());
        EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, certParser_.GetAttestationSecurityLevel());
        return true;
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
