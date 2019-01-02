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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/identity_credential/1.0/IIdentityCredentialStore.h>
#include <android/hardware/identity_credential/1.0/types.h>
#include <log/log.h>

#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>

#include <map>

using ::android::hardware::keymaster::capability::V1_0::CapabilityType;
using ::android::hardware::keymaster::capability::V1_0::KeymasterCapability;
using ::testing::VtsHalHidlTargetTestEnvBase;

using android::hardware::identity_credential::support::hexdump;

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace test {

/******************************
 *     GENERAL CONSTANTS      *
 ******************************/
constexpr uint8_t kAesGcmIvSize = 12;
constexpr uint8_t kAesGcmTagSize = 16;
constexpr uint8_t kAesGcmKeySize = 16;  // 128 bit keys
constexpr uint8_t kECKeySize = 32;      // 128 bit keys

/******************************
 * TEST DATA FOR PROVISIONING *
 ******************************/
struct EntryData {
    EntryData(std::string nameSpace, std::string name, bool directlyAvailable)
        : nameSpace(nameSpace), name(name), directlyAvailable(directlyAvailable) {}
    EntryData(std::string nameSpace, std::string name, std::string string, bool directlyAvailable)
        : EntryData(nameSpace, name, directlyAvailable) {
        value.textString(string);
    }
    EntryData(std::string nameSpace, std::string name, vector<uint8_t> byteString,
              bool directlyAvailable)
        : EntryData(nameSpace, name, directlyAvailable) {
        value.byteString(byteString);
    }
    EntryData(std::string nameSpace, std::string name, bool boolVal, bool directlyAvailable)
        : EntryData(nameSpace, name, directlyAvailable) {
        value.booleanValue(boolVal);
    }
    EntryData(std::string nameSpace, std::string name, uint64_t intVal, bool directlyAvailable)
        : EntryData(nameSpace, name, directlyAvailable) {
        value.integer(intVal);
    }

    std::string nameSpace;
    std::string name;
    EntryValue value;
    bool directlyAvailable;
};

const EntryData testEntry1{"PersonalData", "Last name", std::string("Turing"), false};

const EntryData testEntry2{"PersonalData", "Birth date", std::string("19120623"), false};

const EntryData testEntry3{"PersonalData", "First name", std::string("Alan"), false};

const EntryData testEntry4{"PersonalData", "Home address",
                           std::string("Maida Vale, London, England"), false};

const EntryData testLargeEntry1{
    "Image", "Portrait image",
    hidl_vec<uint8_t>{1, 2, 3, 4},  // TODO: change to actual large image data
    false};

const std::vector<std::pair<EntryData, hidl_vec<uint8_t>>> testEntries{{testEntry1, {1}},
                                                                       {testEntry2, {2}},
                                                                       {testEntry3, {3}},
                                                                       {testEntry4, {0, 1}},
                                                                       {testLargeEntry1, {1, 3}}};

const std::vector<uint16_t> nrOrEntriesInNamespaces = {
    static_cast<uint16_t>(testEntries.size() - 1), 1u};

struct TestProfile {
    uint8_t id;
    hidl_vec<uint8_t> readerAuthPubKey;
    uint64_t capabilityId;
    CapabilityType capabilityType;
    uint64_t timeout;
};

// Profile 1 (reader authentication)
const TestProfile testProfile1 = {0u,
                                  {0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                                   0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                                   0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F},
                                  0u,
                                  CapabilityType::NOT_APPLICABLE,
                                  0u};

// Profile 2 (user authentication)
const TestProfile testProfile2 = {1u, {}, 0x1234567890ABCDEF, CapabilityType::ANY, 100u};

// Profile 3 (user + reader authentication)
const TestProfile testProfile3 = {2u,
                                  {0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                                   0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
                                   0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F},
                                  0x1234567890ABCDEF,
                                  CapabilityType::ANY,
                                  100u};

// Profile 4 (no authentication)
const TestProfile testProfile4 = {3u, {}, 0u, CapabilityType::NOT_APPLICABLE, 0u};

const std::vector<TestProfile> testProfiles{testProfile1, testProfile2, testProfile3, testProfile4};

/************************************
 *   TEST DATA FOR AUTHENTICATION
 ************************************/
// Test authentication token for user authentication
const KeymasterCapability testAuthToken{
    123456,
    {1},
    CapabilityType::ANY,
    666666,
    hidl_vec<uint8_t>{0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF}};

/**
 * Test request data
 * {
 *   "SessionTranscript": [
 *           h'41414141414141414141414141414141',
 *           h'4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F4F'
 *   ],
 *   "Request": {
 *       "DocType": "org.iso18013.mdl",
 *       "PersonalData": [
 *           "Last name",
 *           "Birth date",
 *           "First Name",
 *           "Home Address"
 *       ],
 *       "Image": [
 *           "Portrait image"
 *       ]
 *   }
 * }
 */
const hidl_vec<uint8_t> testRequestData = {
    0xA2, 0x71, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6F, 0x6E, 0x54, 0x72, 0x61, 0x6E, 0x73, 0x63, 0x72,
    0x69, 0x70, 0x74, 0x82, 0x50, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x58, 0x20, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
    0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F,
    0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x67, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0xA3,
    0x67, 0x44, 0x6F, 0x63, 0x54, 0x79, 0x70, 0x65, 0x70, 0x6F, 0x72, 0x67, 0x2E, 0x69, 0x73, 0x6F,
    0x31, 0x38, 0x30, 0x31, 0x33, 0x2E, 0x6D, 0x64, 0x6C, 0x6C, 0x50, 0x65, 0x72, 0x73, 0x6F, 0x6E,
    0x61, 0x6C, 0x44, 0x61, 0x74, 0x61, 0x84, 0x69, 0x4C, 0x61, 0x73, 0x74, 0x20, 0x6E, 0x61, 0x6D,
    0x65, 0x6A, 0x42, 0x69, 0x72, 0x74, 0x68, 0x20, 0x64, 0x61, 0x74, 0x65, 0x6A, 0x46, 0x69, 0x72,
    0x73, 0x74, 0x20, 0x4E, 0x61, 0x6D, 0x65, 0x6C, 0x48, 0x6F, 0x6D, 0x65, 0x20, 0x41, 0x64, 0x64,
    0x72, 0x65, 0x73, 0x73, 0x65, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x81, 0x6E, 0x50, 0x6F, 0x72, 0x74,
    0x72, 0x61, 0x69, 0x74, 0x20, 0x69, 0x6D, 0x61, 0x67, 0x65};

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
        std::string serviceName = IdentityCredentialStoreHidlEnvironment::Instance()
                                      ->getServiceName<IIdentityCredentialStore>("default");
        ASSERT_FALSE(serviceName.empty());

        credentialstore_ =
            ::testing::VtsHalHidlTargetTestBase::getService<IIdentityCredentialStore>(serviceName);

        ASSERT_NE(credentialstore_, nullptr);

        credentialstore_->getHardwareInformation([&](ResultCode hidl_error,
                                                     const hidl_string& credentialStoreName,
                                                     const hidl_string& credentialStoreAuthorName,
                                                     uint32_t chunkSize) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_GT(credentialStoreName.size(), 0u);
            ASSERT_GT(credentialStoreAuthorName.size(), 0u);
            ASSERT_GE(chunkSize, 256u);  // Chunk sizes smaller than APDU buffer won't be supported

            mMaxChunkSize = chunkSize;
        });
    }
    virtual void TearDown() override {}

#if 0
    ResultCode CreateCredential(hidl_vec<uint8_t>* out_credentialBlob) {
        ResultCode error;
        hidl_vec<uint8_t> empty;

        credentialstore_->createCredential(
            "NewCredential", false,
            [&](ResultCode hidl_error, const sp<IWritableIdentityCredential>& newCredential) {
                error = hidl_error;
                ASSERT_NE(newCredential, nullptr);

                newCredential->startPersonalization(
                    empty, empty, testProfiles.size(), testEntries.size(),
                    [&](ResultCode hidl_error, const hidl_vec<uint8_t>& /* certificate */,
                        const hidl_vec<uint8_t>& credentialBlob) {
                        ASSERT_EQ(ResultCode::OK, hidl_error);

                        *out_credentialBlob = credentialBlob;
                    });
            });
        return error;
    }
#endif
    uint32_t mMaxChunkSize = 0;

    sp<IIdentityCredentialStore> credentialstore_;
};

/***************************************************
 *                  TEST CASES                     *
 ***************************************************/
TEST_F(IdentityCredentialStoreHidlTest, HardwareConfiguration) {
    ALOGD("Test HardwareInformation");
    credentialstore_->getHardwareInformation(
        [&](ResultCode hidl_error, const hidl_string& credentialStoreName,
            const hidl_string& credentialStoreAuthorName, uint32_t chunkSize) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_GT(credentialStoreName.size(), 0u);
            ASSERT_GT(credentialStoreAuthorName.size(), 0u);
            ASSERT_GE(chunkSize, 256u);  // Chunk sizes smaller than APDU buffer won't be supported
        });
}

TEST_F(IdentityCredentialStoreHidlTest, CreateCredential) {
    ALOGD("Test CreateCredential");
    credentialstore_->createCredential(
        "NewCredential", false,
        [&](ResultCode hidl_error, const sp<IWritableIdentityCredential>& newCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_NE(newCredential, nullptr);
        });
}

TEST_F(IdentityCredentialStoreHidlTest, ProvisionTestCredential) {
    ALOGD("Test Provisioning Credential");

    sp<IWritableIdentityCredential> writeableCredential;

    hidl_vec<uint8_t> empty{0};

    credentialstore_->createCredential(
        "TestCredential", true,
        [&](ResultCode hidl_error, const sp<IWritableIdentityCredential>& newCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            writeableCredential = newCredential;
        });
    ASSERT_NE(writeableCredential, nullptr);

    // TODO: set these
    std::vector<uint8_t> attestationApplicationId;
    std::vector<uint8_t> attestationChallenge;
    std::vector<uint8_t> attestationCertificate;
    writeableCredential->getAttestationCertificate(
        attestationApplicationId, attestationChallenge,
        [&](ResultCode hidl_error, const hidl_vec<uint8_t>& returnedAttestationCertificate) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            attestationCertificate = returnedAttestationCertificate;
        });
    // TODO: check attestationCertificate

    ASSERT_EQ(ResultCode::OK, writeableCredential->startPersonalization(
                                  testProfiles.size(), {} /* TODO: entryCounts */));

    std::vector<SecureAccessControlProfile> returnedSecureProfiles;
    for (const auto& testProfile : testProfiles) {
        writeableCredential->addAccessControlProfile(
            testProfile.id, testProfile.readerAuthPubKey, testProfile.capabilityId,
            testProfile.capabilityType, testProfile.timeout,
            [&](ResultCode hidl_error, SecureAccessControlProfile profile) {
                ASSERT_EQ(ResultCode::OK, hidl_error);

                ASSERT_EQ(testProfile.id, profile.id);
                ASSERT_EQ(testProfile.readerAuthPubKey, profile.readerAuthPubKey);
                ASSERT_EQ(testProfile.capabilityId, profile.capabilityId);
                ASSERT_EQ(testProfile.capabilityType, profile.capabilityType);
                ASSERT_EQ(testProfile.timeout, profile.timeout);

                ASSERT_EQ(kAesGcmTagSize + kAesGcmIvSize, profile.mac.size());
                // TODO check mac

                returnedSecureProfiles.push_back(profile);
            });
    }

    // Uses EntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    std::map<const EntryData*, vector<uint8_t>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        uint32_t entrySize = 0;
        if (entry.first.value.getDiscriminator() == EntryValue::hidl_discriminator::byteString) {
            entrySize = entry.first.value.byteString().size();
        } else if (entry.first.value.getDiscriminator() ==
                   EntryValue::hidl_discriminator::textString) {
            entrySize = entry.first.value.textString().size();
        }

        writeableCredential->beginAddEntry(entry.second, entry.first.nameSpace, entry.first.name,
                                           entry.first.directlyAvailable, entrySize);

        writeableCredential->addEntryValue(
            entry.first.value, [&](ResultCode hidl_error, hidl_vec<uint8_t> encryptedContent) {
                ASSERT_EQ(ResultCode::OK, hidl_error);
                ASSERT_GT(encryptedContent.size(), 0u);
                // TODO check the encrypted data

                encryptedBlobs[&entry.first] = encryptedContent;
            });
    }

    std::vector<uint8_t> credentialBlob;
    std::vector<uint8_t> signature;
    writeableCredential->finishAddingEntries([&](ResultCode hidl_error,
                                                 const hidl_vec<uint8_t>& returnedCredentialBlob,
                                                 hidl_vec<uint8_t> returnedSignature) {
        ASSERT_EQ(ResultCode::OK, hidl_error);
        credentialBlob = returnedCredentialBlob;
        signature = returnedSignature;
    });
    // TODO: check signature and credentialBlob

    writeableCredential = nullptr;

    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    sp<IIdentityCredential> credential;
    credentialstore_->getCredential(
        credentialBlob, [&](ResultCode hidl_error, const sp<IIdentityCredential>& newCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            credential = newCredential;
        });
    ASSERT_NE(credential, nullptr);

    support::AuthenticatedDataBuilder authenticatedDataBuilder;
    authenticatedDataBuilder.reset("org.iso.18013-5.2019.mdl", {});

    // TODO: call createEphemeralKeyPair?

    StartRetrievalArguments startArguments;
    startArguments.accessControlProfiles = returnedSecureProfiles;
    startArguments.authToken = testAuthToken;
    startArguments.requestCounts = nrOrEntriesInNamespaces;
    // TODO: set other StartRetrievalArguments as appropriate

    ASSERT_EQ(ResultCode::OK, credential->startRetrieval(startArguments));

    for (const auto& entry : testEntries) {
        const EntryData& entryData = entry.first;
        const vector<uint8_t>& profileIds = entry.second;
        uint32_t entrySize = 0;  // TODO: calculate
        ResultCode hidl_error = credential->startRetrieveEntryValue(
            entryData.nameSpace, entryData.name, entrySize, profileIds);
        ASSERT_EQ(ResultCode::OK, hidl_error);

        auto it = encryptedBlobs.find(&entry.first);
        ASSERT_NE(it, encryptedBlobs.end());
        const vector<uint8_t>& encryptedBlob = it->second;

        credential->retrieveEntryValue(
            encryptedBlob, [&](ResultCode hidl_error, const EntryValue& value) {
                ASSERT_EQ(ResultCode::OK, hidl_error);
                ASSERT_EQ(value.getDiscriminator(), entryData.value.getDiscriminator());
                switch (value.getDiscriminator()) {
                    case EntryValue::hidl_discriminator::integer:
                        EXPECT_EQ(value.integer(), entryData.value.integer());
                        break;
                    case EntryValue::hidl_discriminator::textString:
                        EXPECT_EQ(value.textString(), entryData.value.textString());
                        break;
                    case EntryValue::hidl_discriminator::byteString:
                        EXPECT_EQ(value.byteString(), entryData.value.byteString());
                        break;
                    case EntryValue::hidl_discriminator::booleanValue:
                        EXPECT_EQ(value.booleanValue(), entryData.value.booleanValue());
                        break;
                }
                authenticatedDataBuilder.addDataItem(entryData.nameSpace, entryData.name, value);
            });
    }

    std::vector<uint8_t> encodedCbor;
    ASSERT_TRUE(authenticatedDataBuilder.getEncodedCbor(encodedCbor));
    std::string cborPretty;
    ASSERT_TRUE(support::cborPrettyPrint(encodedCbor, cborPretty));
    ASSERT_EQ(
        "{\n"
        "  'Response' : {\n"
        "    'org.iso.18013-5.2019.mdl' : {\n"
        "      'PersonalData' : {\n"
        "        'Last name' : 'Turing',\n"
        "        'Birth date' : '19120623',\n"
        "        'First name' : 'Alan',\n"
        "        'Home address' : 'Maida Vale, London, England'\n"
        "      },\n"
        "      'Image' : {\n"
        "        'Portrait image' : {0x01, 0x02, 0x03, 0x04}\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}",
        cborPretty);

    std::vector<uint8_t> generatedSigningKeyBlob;
    std::vector<uint8_t> generatedSigningKeyCertificate;
    credential->generateSigningKeyPair(
        KeyType::EC_NIST_P_256, [&](ResultCode hidl_error, const hidl_vec<uint8_t> signingKeyBlob,
                                    const hidl_vec<uint8_t> signingKeyCertificate) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            generatedSigningKeyBlob = signingKeyBlob;
            generatedSigningKeyCertificate = signingKeyCertificate;
        });

    vector<uint8_t> retrievalSignature;
    vector<uint8_t> previousAuditSignatureHash;
    credential->finishRetrieval(generatedSigningKeyBlob, previousAuditSignatureHash,
                                [&](ResultCode hidl_error, const hidl_vec<uint8_t> signature,
                                    const AuditLogEntry& auditLogEntry) {
                                    ASSERT_EQ(ResultCode::OK, hidl_error);
                                    retrievalSignature = signature;
                                    ASSERT_GT(signature.size(), 0);
                                });
    EXPECT_TRUE(support::checkSignature(support::sha256(encodedCbor), retrievalSignature,
                                        generatedSigningKeyCertificate));
}

#if 0
TEST_F(IdentityCredentialStoreHidlTest, GetCredential) {
    ALOGD("GetCredentialTest ");
    sp<IIdentityCredential> readCredential;
    hidl_vec<uint8_t> empty{0};
    hidl_vec<uint8_t> testCredentialBlob{0};

    CreateCredential(&testCredentialBlob);

    ASSERT_GT(testCredentialBlob.size(), 0u);
    credentialstore_->getCredential(
        testCredentialBlob,
        [&](ResultCode hidl_error, const sp<IIdentityCredential>& loadedCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_NE(loadedCredential, nullptr);
            readCredential = loadedCredential;
        });
}

TEST_F(IdentityCredentialStoreHidlTest, TestCreateEphemeralKey) {
    ALOGD("CreateEphemeralKeyTest");
    sp<IIdentityCredential> readCredential;

    hidl_vec<uint8_t> testCredentialBlob{0};

    CreateCredential(&testCredentialBlob);

    credentialstore_->getCredential(
        testCredentialBlob,
        [&](ResultCode hidl_error, const sp<IIdentityCredential>& loadedCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_NE(loadedCredential, nullptr);
            readCredential = loadedCredential;
        });
    ASSERT_NE(readCredential, nullptr);

    readCredential->createEphemeralKeyPair(
        KeyType::EC_NIST_P_256,
        [&](const hidl_vec<uint8_t>& ephemeralKey) { ASSERT_GT(ephemeralKey.size(), 0u); });
}

TEST_F(IdentityCredentialStoreHidlTest, TestRetrieveEntries) {
    ALOGD("TestRetrieveEntires");
    sp<IIdentityCredential> readCredential;

    hidl_vec<uint8_t> testCredentialBlob{0};
    StartRetrievalArguments startArguments;

    CreateCredential(&testCredentialBlob);

    credentialstore_->getCredential(
        testCredentialBlob,
        [&](ResultCode hidl_error, const sp<IIdentityCredential>& loadedCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_NE(loadedCredential, nullptr);
            readCredential = loadedCredential;
        });
    ASSERT_NE(readCredential, nullptr);

    readCredential->createEphemeralKeyPair(
        KeyType::EC_NIST_P_256,
        [&](const hidl_vec<uint8_t>& ephemeralKey) { ASSERT_GT(ephemeralKey.size(), 0u); });

    std::vector<SecureAccessControlProfile> secureTestProfiles;

    // Specify secure access control profile
    for (auto& profile : testProfiles) {
        SecureAccessControlProfile sProfile;
        sProfile.id = profile.id;
        sProfile.readerAuthPubKey = profile.readerAuthPubKey;
        sProfile.capabilityId = profile.capabilityId;
        sProfile.capabilityType = profile.capabilityType;
        sProfile.timeout = profile.timeout;
        // TODO(hoelzl) dynamically compute mac with test credential keys
        sProfile.mac = hidl_vec<uint8_t>{0, 2, 3, 4, 5};

        secureTestProfiles.push_back(sProfile);
    }
    startArguments.accessControlProfiles = secureTestProfiles;

    // User authentication token
    startArguments.authToken = testAuthToken;

    // Two namespaces
    startArguments.requestCounts = nrOrEntriesInNamespaces;

    // Test request
    startArguments.requestData = testRequestData;

    readCredential->startRetrieval(startArguments,
                                   [&](ResultCode hidl_error, const hidl_vec<uint8_t> failedIds) {
                                       ASSERT_EQ(ResultCode::OK, hidl_error);
                                       ASSERT_NE(failedIds, 0u);
                                   });
}
#endif

}  // namespace test
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::android::base::InitLogging(argv, &android::base::StderrLogger);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
