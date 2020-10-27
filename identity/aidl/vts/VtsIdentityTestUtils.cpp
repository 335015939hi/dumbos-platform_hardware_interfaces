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

bool setupWritableCredential(sp<IWritableIdentityCredential>& writableCredential,
                             sp<IIdentityCredentialStore>& credentialStore, bool testCredential) {
    if (credentialStore == nullptr) {
        return false;
    }

    string docType = "org.iso.18013-5.2019.mdl";
    Status result = credentialStore->createCredential(docType, testCredential, &writableCredential);

    if (result.isOk() && writableCredential != nullptr) {
        return true;
    } else {
        return false;
    }
}

optional<vector<uint8_t>> generateReaderCertificate(string serialDecimal) {
    vector<uint8_t> privKey;
    return generateReaderCertificate(serialDecimal, &privKey);
}

optional<vector<uint8_t>> generateReaderCertificate(string serialDecimal,
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

optional<vector<SecureAccessControlProfile>> addAccessControlProfiles(
        sp<IWritableIdentityCredential>& writableCredential,
        const vector<TestProfile>& testProfiles) {
    Status result;

    vector<SecureAccessControlProfile> secureProfiles;

    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        int64_t secureUserId = testProfile.userAuthenticationRequired ? 66 : 0;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, secureUserId, &profile);

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
bool addEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
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

void setImageData(vector<uint8_t>& image) {
    image.resize(256 * 1024 - 10);
    for (size_t n = 0; n < image.size(); n++) {
        image[n] = (uint8_t)n;
    }
}

bool validateAttestationCertificate(const vector<Certificate>& inputCertificates,
                                    const vector<uint8_t>& expectedChallenge,
                                    const vector<uint8_t>& expectedAppId,
                                    const HardwareInformation& hwInfo, bool isTestCredential) {
    AttestationCertificateParser parser(inputCertificates);
    bool ret = parser.parse();
    EXPECT_TRUE(ret);
    if (!ret) {
        return false;
    }

    // The default HAL is implemented in software so it will report security levels
    // in KM_SECURITY_LEVEL_SOFTWARE, not KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT.
    //
    bool isDefaultImpl = false;
    if (hwInfo.credentialStoreName == "Identity Credential Reference Implementation" &&
        hwInfo.credentialStoreAuthorName == "Google") {
        isDefaultImpl = true;
    }

    // Check all the requirements from IWritableIdentityCredential::getAttestationCertificate()...
    //

    //  - version: INTEGER 2 (means v3 certificate).
    EXPECT_EQ(2, parser.getVersion());

    //  - serialNumber: INTEGER 1 (fixed value: same on all certs).
    EXPECT_EQ(1, parser.getSerialNumber());

    //  - signature: must be set to ECDSA.
    EXPECT_EQ(NID_ecdsa_with_SHA256, parser.getSignatureNid());

    //  - subject: CN shall be set to "Android Identity Credential Key". (fixed value:
    //    same on all certs)
    EXPECT_EQ("CN=Android Identity Credential Key", parser.getSubjectName());

    //  - issuer: Same as the subject field of the batch attestation key.
    EXPECT_EQ(parser.getBatchCertSubjectName(), parser.getIssuerName());

    //  - validity: Should be from current time and expire at the same time as the
    //    attestation batch certificate used.
    //
    //  Allow for 10 seconds drift to account for the time drift between Secure HW
    //  and this environment plus the difference between when the certificate was
    //  created and until now
    //
    uint64_t now = time(nullptr);
    int64_t diffSecs = now - parser.getNotBefore();
    int64_t allowDriftSecs = 10;
    EXPECT_LE(-allowDriftSecs, diffSecs);
    EXPECT_GE(allowDriftSecs, diffSecs);
    EXPECT_EQ(parser.getNotAfter(), parser.getBatchNotAfter());

    //  - subjectPublicKeyInfo: must contain attested public key.

    //  - The attestationVersion field in the attestation extension must be at least 3.
    EXPECT_LE(3, parser.getAttestationVersion());

    //  - The attestationSecurityLevel field must be set to either Software (0),
    //    TrustedEnvironment (1), or StrongBox (2) depending on how attestation is
    //    implemented. Only the default AOSP implementation of this HAL may use
    //    value 0 (additionally, this implementation must not be used on production
    //    devices).
    if (isDefaultImpl) {
        EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, parser.getAttestationSecurityLevel());
    } else {
        EXPECT_LE(KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT, parser.getAttestationSecurityLevel());
    }

    //  - The keymasterVersion field in the attestation extension must be set to 10.
    EXPECT_EQ(10, parser.getKeymasterVersion());

    //  - The keymasterSecurityLevel field in the attestation extension must be set to
    //    either Software (0), TrustedEnvironment (1), or StrongBox (2) depending on how
    //    the Trusted Application backing the HAL implementation is implemented. Only
    //    the default AOSP implementation of this HAL may use value 0 (additionally, this
    //    implementation must not be used on production devices)
    if (isDefaultImpl) {
        EXPECT_LE(KM_SECURITY_LEVEL_SOFTWARE, parser.getKeymasterSecurityLevel());
    } else {
        EXPECT_LE(KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT, parser.getKeymasterSecurityLevel());
    }

    //  - The attestationChallenge field must be set to the passed-in challenge.
    vector<uint8_t> attChallenge = parser.getAttestationChallenge();
    EXPECT_EQ(expectedChallenge.size(), attChallenge.size());
    EXPECT_EQ(0, memcmp(expectedChallenge.data(), attChallenge.data(), expectedChallenge.size()));

    //  - The uniqueId field must be empty.
    // TODO
    EXPECT_EQ(0, parser.getAttestationUniqueId().size());
    EXPECT_FALSE(parser.containsHwEnforcedBool(::keymaster::TAG_INCLUDE_UNIQUE_ID));

    //  - The softwareEnforced field in the attestation extension must include
    //    Tag::ATTESTATION_APPLICATION_ID which must be set to the bytes of the passed-in
    //    attestationApplicationId.
    optional<vector<uint8_t>> appId =
            parser.getSwEnforcedBlob(::keymaster::TAG_ATTESTATION_APPLICATION_ID);
    EXPECT_TRUE(appId);
    EXPECT_EQ(expectedAppId.size(), appId.value().size());
    EXPECT_EQ(0, memcmp(expectedAppId.data(), appId.value().data(), expectedAppId.size()));

    //  - The teeEnforced field in the attestation extension must include
    //
    //    - Tag::IDENTITY_CREDENTIAL_KEY which indicates that the key is an Identity
    //      Credential key (which can only sign/MAC very specific messages) and not an Android
    //      Keystore key (which can be used to sign/MAC anything). This must not be set
    //      for test credentials.
    if (isTestCredential) {
        EXPECT_FALSE(parser.containsHwEnforcedBool(::keymaster::TAG_IDENTITY_CREDENTIAL_KEY));
    } else {
        EXPECT_TRUE(parser.containsHwEnforcedBool(::keymaster::TAG_IDENTITY_CREDENTIAL_KEY));
    }

    //    - Tag::PURPOSE must be set to SIGN
    EXPECT_TRUE(parser.containsHwEnforcedEnumRep(::keymaster::TAG_PURPOSE, KM_PURPOSE_SIGN));

    //    - Tag::KEY_SIZE must be set to the appropriate key size, in bits (e.g. 256)
    optional<uint32_t> keySize = parser.getHwEnforcedUint(::keymaster::TAG_KEY_SIZE);
    EXPECT_TRUE(keySize);
    EXPECT_EQ(256, keySize.value());

    //    - Tag::ALGORITHM must be set to EC
    optional<keymaster_algorithm_t> alg = parser.getHwEnforcedEnum(::keymaster::TAG_ALGORITHM);
    EXPECT_TRUE(alg);
    EXPECT_EQ(KM_ALGORITHM_EC, alg.value());

    //    - Tag::NO_AUTH_REQUIRED must be set
    EXPECT_TRUE(parser.containsHwEnforcedBool(::keymaster::TAG_NO_AUTH_REQUIRED));

    //    - Tag::DIGEST must be include SHA_2_256
    EXPECT_TRUE(parser.containsHwEnforcedEnumRep(::keymaster::TAG_DIGEST, KM_DIGEST_SHA_2_256));

    //    - Tag::EC_CURVE must be set to P_256
    optional<keymaster_ec_curve_t> curve = parser.getHwEnforcedEnum(::keymaster::TAG_EC_CURVE);
    EXPECT_TRUE(curve);
    EXPECT_EQ(KM_EC_CURVE_P_256, curve.value());

    //    - Tag::ROOT_OF_TRUST must be set
    EXPECT_TRUE(parser.hasHwEnforcedRootOfTrust());

    //    - Tag::OS_VERSION and Tag::OS_PATCHLEVEL must be set
    optional<uint32_t> osVersion = parser.getHwEnforcedUint(::keymaster::TAG_OS_VERSION);
    EXPECT_TRUE(osVersion);
    optional<uint32_t> osPatchLevel = parser.getHwEnforcedUint(::keymaster::TAG_OS_PATCHLEVEL);
    EXPECT_TRUE(osPatchLevel);
    // TODO: we could retrieve osVersion and osPatchLevel from Android itself and compare it
    // with what was reported in the certificate and retrieve just above...

    return true;
}

vector<RequestNamespace> buildRequestNamespaces(const vector<TestEntryData> entries) {
    vector<RequestNamespace> ret;
    RequestNamespace curNs;
    for (const TestEntryData& testEntry : entries) {
        if (testEntry.nameSpace != curNs.namespaceName) {
            if (curNs.namespaceName.size() > 0) {
                ret.push_back(curNs);
            }
            curNs.namespaceName = testEntry.nameSpace;
            curNs.items.clear();
        }

        RequestDataItem item;
        item.name = testEntry.name;
        item.size = testEntry.valueCbor.size();
        item.accessControlProfileIds = testEntry.profileIds;
        curNs.items.push_back(item);
    }
    if (curNs.namespaceName.size() > 0) {
        ret.push_back(curNs);
    }
    return ret;
}

}  // namespace android::hardware::identity::test_utils
