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

#include "VtsIdentityTestUtils.h"

#include <aidl/Gtest.h>
#include <map>

#include "VtsAttestationParserSupport.h"

namespace android::hardware::identity::test_utils {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

bool SetupWritableCredential(sp<IWritableIdentityCredential>& writableCredential,
                             sp<IIdentityCredentialStore>& credentialStore) {
    if (credentialStore == nullptr) {
        return false;
    }

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    Status result = credentialStore->createCredential(docType, testCredential, &writableCredential);

    if (result.isOk() && writableCredential != nullptr) {
        return true;
    } else {
        return false;
    }
}

optional<vector<uint8_t>> GenerateReaderCertificate(string serialDecimal) {
    vector<uint8_t> privKey;
    return GenerateReaderCertificate(serialDecimal, &privKey);
}

optional<vector<uint8_t>> GenerateReaderCertificate(string serialDecimal,
                                                    vector<uint8_t>* outReaderPrivateKey) {
    optional<vector<uint8_t>> readerKeyPKCS8 = support::createEcKeyPair();
    if (!readerKeyPKCS8) {
        return {};
    }

    optional<vector<uint8_t>> readerPublicKey =
            support::ecKeyPairGetPublicKey(readerKeyPKCS8.value());
    optional<vector<uint8_t>> readerKey = support::ecKeyPairGetPrivateKey(readerKeyPKCS8.value());
    if (!readerPublicKey || !readerKey) {
        return {};
    }

    if (outReaderPrivateKey == nullptr) {
        return {};
    }

    *outReaderPrivateKey = readerKey.value();

    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential VTS Test";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    return support::ecPublicKeyGenerateCertificate(readerPublicKey.value(), readerKey.value(),
                                                   serialDecimal, issuer, subject,
                                                   validityNotBefore, validityNotAfter);
}

optional<vector<SecureAccessControlProfile>> AddAccessControlProfiles(
        sp<IWritableIdentityCredential>& writableCredential,
        const vector<TestProfile>& testProfiles) {
    Status result;

    vector<SecureAccessControlProfile> secureProfiles;

    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, 0, &profile);

        // Don't use assert so all errors can be outputed.  Then return
        // instead of exit even on errors so caller can decide.
        EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
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
            return {};
        }

        secureProfiles.push_back(profile);
    }

    return secureProfiles;
}

// Most test expects this function to pass. So we will print out additional
// value if failed so more debug data can be provided.
bool AddEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
              int dataChunkSize, map<const TestEntryData*, vector<vector<uint8_t>>>& encryptedBlobs,
              bool expectSuccess) {
    Status result;
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

            EXPECT_GT(encryptedContent.size(), 0u) << "entry name = " << entry.name
                                                   << ", name space = " << entry.nameSpace << endl;
        }

        if (!result.isOk() || encryptedContent.size() <= 0u) {
            return false;
        }

        encryptedChunks.push_back(encryptedContent);
    }

    encryptedBlobs[&entry] = encryptedChunks;
    return true;
}


void SetImageData(vector<uint8_t>& image) {
    image.resize(256 * 1024 - 10);
    for (size_t n = 0; n < image.size(); n++) {
        image[n] = (uint8_t)n;
    }
}

bool ValidateAttestationCertificate(const vector<Certificate>& inputCertificates,
                                    const vector<uint8_t>& expected_challenge,
                                    const vector<uint8_t>& expected_app_id) {
    AttestationCertificateParser certParser_(inputCertificates);
    bool ret = certParser_.Parse();
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
    EXPECT_LE(10, certParser_.GetKeymasterVersion());
    EXPECT_GT(40, certParser_.GetKeymasterVersion());
    EXPECT_LE(3, certParser_.GetAttestationVersion());

    // Verify the app id matches to whatever we set it to be.
    optional<vector<uint8_t>> appId = certParser_.GetApplicationId();
    if (appId) {
        EXPECT_EQ(expected_app_id.size(), appId.value().size());
        EXPECT_EQ(0, memcmp(expected_app_id.data(), appId.value().data(), expected_app_id.size()));
    } else {
        // app id not found
        EXPECT_EQ(0, expected_app_id.size());
    }

    EXPECT_FALSE(certParser_.IncludeUniqueId());
    EXPECT_TRUE(certParser_.IncludeIdentityCredentialKey());
    // EXPECT_TRUE(certParser_.GetHwEnforcedBool(::keymaster::TAG_IDENTITY_CREDENTIAL_KEY));

    // Verify the challenge always matches in size and data of what is passed
    // in.
    vector<uint8_t> att_challenge = certParser_.GetAttestationChallenge();
    EXPECT_EQ(expected_challenge.size(), att_challenge.size());
    EXPECT_EQ(0,
              memcmp(expected_challenge.data(), att_challenge.data(), expected_challenge.size()));

    // The level is a bit tricky.  It may be different for each hardware.  May
    // need to delete this, unless there is a correlation with the version
    // number.
    EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, certParser_.GetKeymasterSecurityLevel());
    EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, certParser_.GetAttestationSecurityLevel());
    return true;
}

}  // namespace android::hardware::identity::test_utils
