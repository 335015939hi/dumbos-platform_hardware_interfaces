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
namespace test {

using ::android::hardware::identity::V1_0::AuditLogEntry;
using ::android::hardware::identity::V1_0::IIdentityCredential;
using ::android::hardware::identity::V1_0::IIdentityCredentialStore;
using ::android::hardware::identity::V1_0::IWritableIdentityCredential;
using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;
using ::android::hardware::identity::V1_0::SecureAccessControlProfile;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;

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

    HardwareAuthToken authToken = {};

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
    // TODO: Need to check attestationCertificate:
    //
    // - is a chain of at least two certificates
    // - each certificate in the chain is signed by the next one
    // - the root cert is well-known (maybe (probably not) - won't work on non-production devices)
    // - the Android Keystore attestation extension is present, including
    //   - the passed-in challenge is set
    //   - the tag TODO_IC_KEY is set
    //
    // This should probably be done in a separate stand-alone test, not here.
    //
    // Related TODO: The default HAL implementation currently doesn't use Keymaster
    // Attestation so it won't be able to pass any of these tests. It would be
    // worthwhile to update it to use SW Keymaster to achieve this.

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
                0,  // secureUserId
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
            credentialData, [&](const Result& _result, const sp<IIdentityCredential>& _credential) {
                result = _result;
                credential = _credential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(credential, nullptr);

    vector<uint8_t> readerEphemeralKeyPair;
    ASSERT_TRUE(support::createEcKeyPair(readerEphemeralKeyPair));
    vector<uint8_t> readerEphemeralPublicKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(readerEphemeralKeyPair, readerEphemeralPublicKey));
    credential->setReaderEphemeralPublicKey(readerEphemeralPublicKey,
                                            [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

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
    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpacesBytes;
    credential->finishRetrieval(signingKeyBlob, previousAuditSignatureHash,
                                [&](const Result& _result, const hidl_vec<uint8_t> _mac,
                                    const hidl_vec<uint8_t> _deviceNameSpacesBytes,
                                    const AuditLogEntry& /* _auditLogEntry*/) {
                                    result = _result;
                                    mac = _mac;
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
    // The data that is MACed is ["DeviceAuthentication", sessionTranscriptBytes, docType,
    // deviceNameSpacesBytes] so build up that structure
    cppbor::Array deviceAuthentication;
    deviceAuthentication.add("DeviceAuthentication");
    deviceAuthentication.add(sessionTranscript.clone());
    deviceAuthentication.add(docType);
    deviceAuthentication.add(cppbor::Semantic(24, deviceNameSpacesBytes));
    vector<uint8_t> encodedDeviceAuthentication = deviceAuthentication.encode();
    vector<uint8_t> signingPublicKey;
    EXPECT_TRUE(support::certificateChainGetTopMostKey(signingKeyCertificate, signingPublicKey));

    // Derive the key used for MACing.
    vector<uint8_t> readerEphemeralPrivateKey;
    ASSERT_TRUE(support::ecKeyPairGetPrivateKey(readerEphemeralKeyPair, readerEphemeralPrivateKey));
    vector<uint8_t> sharedSecret;
    ASSERT_TRUE(support::ecdh(signingPublicKey, readerEphemeralPrivateKey, sharedSecret));
    vector<uint8_t> salt = {0x00};
    vector<uint8_t> info = {};
    vector<uint8_t> derivedKey;
    ASSERT_TRUE(support::hkdf(sharedSecret, salt, info, 32, derivedKey));
    vector<uint8_t> calculatedMac;
    EXPECT_TRUE(support::coseMac0(derivedKey, {},               // payload
                                  encodedDeviceAuthentication,  // detached content
                                  calculatedMac));
    EXPECT_EQ(mac, calculatedMac);
}

}  // namespace test
}  // namespace identity
}  // namespace hardware
}  // namespace android

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::android::base::InitLogging(argv, &android::base::StderrLogger);
    int status = RUN_ALL_TESTS();
    return status;
}
