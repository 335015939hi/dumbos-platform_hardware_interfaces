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
#include <android/hardware/identity_credential/1.0/IIdentityCredentialStore.h>
#include <android/hardware/identity_credential/1.0/types.h>
#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>

using ::android::hardware::keymaster::capability::V1_0::CapabilityType;
using ::android::hardware::keymaster::capability::V1_0::KeymasterCapability;
using ::testing::VtsHalHidlTargetTestEnvBase;

using std::string;
using std::vector;
// using NameSpaceAndDataNames = std::make_pair<string, vector<string>>;

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace test {

// ---------------------------------------------------------------------------
// Test Data.
// ---------------------------------------------------------------------------

struct TestEntryData {
    TestEntryData(string nameSpace, string name, bool directlyAvailable, vector<uint8_t> profileIds)
        : nameSpace(nameSpace),
          name(name),
          directlyAvailable(directlyAvailable),
          profileIds(profileIds) {}

    TestEntryData(string nameSpace, string name, string string, bool directlyAvailable,
                  vector<uint8_t> profileIds)
        : TestEntryData(nameSpace, name, directlyAvailable, profileIds) {
        value.textString(string);
    }
    TestEntryData(string nameSpace, string name, vector<uint8_t> byteString, bool directlyAvailable,
                  vector<uint8_t> profileIds)
        : TestEntryData(nameSpace, name, directlyAvailable, profileIds) {
        value.byteString(byteString);
    }
    TestEntryData(string nameSpace, string name, bool boolVal, bool directlyAvailable,
                  vector<uint8_t> profileIds)
        : TestEntryData(nameSpace, name, directlyAvailable, profileIds) {
        value.booleanValue(boolVal);
    }
    TestEntryData(string nameSpace, string name, uint64_t intVal, bool directlyAvailable,
                  vector<uint8_t> profileIds)
        : TestEntryData(nameSpace, name, directlyAvailable, profileIds) {
        value.integer(intVal);
    }

    string nameSpace;
    string name;
    EntryValue value;
    bool directlyAvailable;
    vector<uint8_t> profileIds;
};

struct TestProfile {
    uint8_t id;
    hidl_vec<uint8_t> readerAuthPubKey;
    uint64_t capabilityId;
    CapabilityType capabilityType;
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
            ::testing::VtsHalHidlTargetTestBase::getService<IIdentityCredentialStore>(serviceName);
        ASSERT_NE(credentialStore_, nullptr);

        credentialStore_->getHardwareInformation([&](ResultCode hidl_error,
                                                     const hidl_string& credentialStoreName,
                                                     const hidl_string& credentialStoreAuthorName,
                                                     uint32_t chunkSize) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_GT(credentialStoreName.size(), 0u);
            ASSERT_GT(credentialStoreAuthorName.size(), 0u);
            ASSERT_GE(chunkSize, 256u);  // Chunk sizes smaller than APDU buffer won't be supported

            maxChunkSize_ = chunkSize;
        });
    }
    virtual void TearDown() override {}

    uint32_t maxChunkSize_ = 0;

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_F(IdentityCredentialStoreHidlTest, HardwareConfiguration) {
    credentialStore_->getHardwareInformation(
        [&](ResultCode hidl_error, const hidl_string& credentialStoreName,
            const hidl_string& credentialStoreAuthorName, uint32_t chunkSize) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ASSERT_GT(credentialStoreName.size(), 0u);
            ASSERT_GT(credentialStoreAuthorName.size(), 0u);
            ASSERT_GE(chunkSize, 256u);  // Chunk sizes smaller than APDU buffer won't be supported
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
    const vector<TestProfile> testProfiles = {
        // Profile 0 (reader authentication)
        {0, readerCertificate, 0, CapabilityType::NOT_APPLICABLE, 0},
        // Profile 1 (no authentication)
        {1, {}, 0, CapabilityType::NOT_APPLICABLE, 0}};

    uint64_t timestamp;
    ASSERT_TRUE(support::getMillisecondsSinceBoot(timestamp));
    const KeymasterCapability authToken = {123456,  // |challenge| is unused
                                           {},      // |ids| is unused
                                           CapabilityType::NOT_APPLICABLE,
                                           timestamp,
                                           vector<uint8_t>{0x01}};  // |secureToken| is unused

    // Here's the actual test data:
    const vector<TestEntryData> testEntries = {
        {"PersonalData", "Last name", string("Turing"), false, vector<uint8_t>{0, 1}},
        {"PersonalData", "Birth date", string("19120623"), false, vector<uint8_t>{0, 1}},
        {"PersonalData", "First name", string("Alan"), false, vector<uint8_t>{0, 1}},
        {"PersonalData", "Home address", string("Maida Vale, London, England"), false,
         vector<uint8_t>{0}},
        {"Image", "Portrait image", portraitImage, false, vector<uint8_t>{0, 1}},
    };
    const vector<uint16_t> testEntriesEntryCounts = {static_cast<uint16_t>(testEntries.size() - 1),
                                                     1u};

    string cborPretty;
    sp<IWritableIdentityCredential> writeableCredential;

    hidl_vec<uint8_t> empty{0};

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    credentialStore_->createCredential(
        docType, testCredential,
        [&](ResultCode hidl_error, const sp<IWritableIdentityCredential>& newCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            writeableCredential = newCredential;
        });
    ASSERT_NE(writeableCredential, nullptr);

    string id = "attestationApplicationId";
    string challenge = "attestationChallenge";
    vector<uint8_t> attestationApplicationId(id.begin(), id.end());
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<uint8_t> attestationCertificate;
    writeableCredential->getAttestationCertificate(
        attestationApplicationId, attestationChallenge,
        [&](ResultCode hidl_error, const hidl_vec<uint8_t>& returnedAttestationCertificate) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            attestationCertificate = returnedAttestationCertificate;
        });
    // TODO: check attestationCertificate

    support::SignedDataBuilder signedDataBuilder;
    signedDataBuilder.reset(docType, testCredential);

    ASSERT_EQ(ResultCode::OK, writeableCredential->startPersonalization(testProfiles.size(),
                                                                        testEntriesEntryCounts));

    vector<SecureAccessControlProfile> returnedSecureProfiles;
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
                ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
                returnedSecureProfiles.push_back(profile);
                signedDataBuilder.addAccessControlProfile(profile);
            });
    }

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    std::map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        uint32_t entrySize = 0;
        if (entry.value.getDiscriminator() == EntryValue::hidl_discriminator::byteString) {
            entrySize = entry.value.byteString().size();
        } else if (entry.value.getDiscriminator() == EntryValue::hidl_discriminator::textString) {
            entrySize = entry.value.textString().size();
        }

        writeableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                           entry.directlyAvailable, entrySize);

        vector<EntryValue> chunks = support::entrySplitIntoChunks(entry.value, maxChunkSize_);
        vector<vector<uint8_t>> encryptedChunks;
        for (const auto& chunk : chunks) {
            writeableCredential->addEntryValue(
                chunk, [&](ResultCode hidl_error, hidl_vec<uint8_t> encryptedContent) {
                    ASSERT_EQ(ResultCode::OK, hidl_error);
                    ASSERT_GT(encryptedContent.size(), 0u);
                    encryptedChunks.push_back(encryptedContent);
                });
        }
        encryptedBlobs[&entry] = encryptedChunks;

        signedDataBuilder.addEntry(entry.nameSpace, entry.name, entry.profileIds, entry.value,
                                   entry.directlyAvailable);
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> signature;
    writeableCredential->finishAddingEntries([&](ResultCode hidl_error,
                                                 const hidl_vec<uint8_t>& returnedCredentialBlob,
                                                 hidl_vec<uint8_t> returnedSignature) {
        ASSERT_EQ(ResultCode::OK, hidl_error);
        credentialData = returnedCredentialBlob;
        signature = returnedSignature;
    });
    // TODO: inspect credentialData

    vector<uint8_t> encodedCbor;
    ASSERT_TRUE(signedDataBuilder.getEncodedCbor(encodedCbor));
    ASSERT_TRUE(support::cborPrettyPrint(encodedCbor, cborPretty, 32, {"readerAuthPubKey"}));
    EXPECT_EQ(
        "{\n"
        "  'docType' : 'org.iso.18013-5.2019.mdl',\n"
        "  'accessControlProfiles' : [{\n"
        "      'id' : 0,\n"
        "      'readerAuthPubKey' : <not printed>\n"
        "    }, {\n"
        "      'id' : 1\n"
        "    }],\n"
        "  'namespaces' : {\n"
        "    'PersonalData' : [{\n"
        "        'name' : 'Last name',\n"
        "        'accessControlProfiles' : [0, 1],\n"
        "        'value' : 'Turing',\n"
        "        'directlyAvailable' : false\n"
        "      }, {\n"
        "        'name' : 'Birth date',\n"
        "        'accessControlProfiles' : [0, 1],\n"
        "        'value' : '19120623',\n"
        "        'directlyAvailable' : false\n"
        "      }, {\n"
        "        'name' : 'First name',\n"
        "        'accessControlProfiles' : [0, 1],\n"
        "        'value' : 'Alan',\n"
        "        'directlyAvailable' : false\n"
        "      }, {\n"
        "        'name' : 'Home address',\n"
        "        'accessControlProfiles' : [0],\n"
        "        'value' : 'Maida Vale, London, England',\n"
        "        'directlyAvailable' : false\n"
        "      }],\n"
        "    'Image' : [{\n"
        "        'name' : 'Portrait image',\n"
        "        'accessControlProfiles' : [0, 1],\n"
        "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
        "        'directlyAvailable' : false\n"
        "      }]\n"
        "  },\n"
        "  'testCredential' : true\n"
        "}",
        cborPretty);
    EXPECT_TRUE(support::checkEcDsaSignature(support::sha256(encodedCbor), signature,
                                             attestationCertificate));

    writeableCredential = nullptr;

    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    sp<IIdentityCredential> credential;
    credentialStore_->getCredential(
        credentialData, [&](ResultCode hidl_error, const sp<IIdentityCredential>& newCredential) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            credential = newCredential;
        });
    ASSERT_NE(credential, nullptr);

    vector<uint8_t> ephemeralKeyPair;
    credential->createEphemeralKeyPair(
        [&](ResultCode hidl_error, const hidl_vec<uint8_t>& keyPair) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            ephemeralKeyPair = keyPair;
        });
    vector<uint8_t> ephemeralPublicKey;
    ASSERT_TRUE(support::ecKeyPairGetPublicKey(ephemeralKeyPair, ephemeralPublicKey));

    // Calculate requestData field and sign it with the reader key.
    auto sessionTranscript = support::CnCborPtr(cn_cbor_map_create(nullptr));
    ASSERT_TRUE(support::cborMapPutStringBStr(sessionTranscript.get(), "EphemeralPublicKey",
                                              ephemeralPublicKey.data(),
                                              ephemeralPublicKey.size()));
    vector<uint8_t> encodedSessionTranscript;
    ASSERT_TRUE(support::cborEncode(sessionTranscript.get(), encodedSessionTranscript));
    vector<uint8_t> requestDataCbor;
    ASSERT_TRUE(support::generateRequestData(
        encodedSessionTranscript,
        {{"org.iso.18013-5.2019.mdl",
          {{"PersonalData", {"Last name", "Birth date", "First name", "Home address"}},
           {"Image", {"Portrait image"}}}},
         {"com.android.identity_credential.example.library_card",
          {{"PersonalData",
            {
                "Last name",
                "First name",
            }},
           {"Image", {"Portrait image"}}}}},
        requestDataCbor));
    ASSERT_TRUE(support::cborPrettyPrint(requestDataCbor, cborPretty, 32, {"EphemeralPublicKey"}));
    EXPECT_EQ(
        "{\n"
        "  'SessionTranscript' : {\n"
        "    'EphemeralPublicKey' : <not printed>\n"
        "  },\n"
        "  'Request' : [{\n"
        "      'DocType' : 'org.iso.18013-5.2019.mdl',\n"
        "      'PersonalData' : ['Last name', 'Birth date', 'First name', 'Home address'],\n"
        "      'Image' : ['Portrait image']\n"
        "    }, {\n"
        "      'DocType' : 'com.android.identity_credential.example.library_card',\n"
        "      'PersonalData' : ['Last name', 'First name'],\n"
        "      'Image' : ['Portrait image']\n"
        "    }]\n"
        "}",
        cborPretty);
    vector<uint8_t> readerSignature;
    ASSERT_TRUE(support::signEcDsa(readerKey, requestDataCbor, readerSignature));

    StartRetrievalArguments startArguments;
    startArguments.accessControlProfiles = returnedSecureProfiles;
    startArguments.authToken = authToken;
    startArguments.requestData = requestDataCbor;
    startArguments.readerSignature = readerSignature;
    startArguments.requestCounts = testEntriesEntryCounts;
    ASSERT_EQ(ResultCode::OK, credential->startRetrieval(startArguments));

    // Now retrieve all data and while doing so, build the AuthenticatedData
    // CBOR that will be signed.
    support::AuthenticatedDataBuilder authenticatedDataBuilder;
    authenticatedDataBuilder.reset("org.iso.18013-5.2019.mdl", encodedSessionTranscript);
    for (const auto& entry : testEntries) {
        uint32_t entrySize;
        switch (entry.value.getDiscriminator()) {
            case EntryValue::hidl_discriminator::textString:
                entrySize = entry.value.textString().size();
                break;
            case EntryValue::hidl_discriminator::byteString:
                entrySize = entry.value.byteString().size();
                break;
            default:
                entrySize = 0;
                break;
        }
        ResultCode hidl_error = credential->startRetrieveEntryValue(entry.nameSpace, entry.name,
                                                                    entrySize, entry.profileIds);
        ASSERT_EQ(ResultCode::OK, hidl_error);

        auto it = encryptedBlobs.find(&entry);
        ASSERT_NE(it, encryptedBlobs.end());
        const vector<vector<uint8_t>>& encryptedChunks = it->second;

        vector<EntryValue> chunks = support::entrySplitIntoChunks(entry.value, maxChunkSize_);
        ASSERT_EQ(chunks.size(), encryptedChunks.size());
        size_t n = 0;
        for (const auto& chunk : chunks) {
            const vector<uint8_t>& encryptedContent = encryptedChunks[n];

            credential->retrieveEntryValue(
                encryptedContent, [&](ResultCode hidl_error, const EntryValue& value) {
                    ASSERT_EQ(ResultCode::OK, hidl_error);
                    ASSERT_EQ(value.getDiscriminator(), entry.value.getDiscriminator());
                    switch (value.getDiscriminator()) {
                        case EntryValue::hidl_discriminator::integer:
                            EXPECT_EQ(value.integer(), chunk.integer());
                            break;
                        case EntryValue::hidl_discriminator::textString:
                            EXPECT_EQ(value.textString(), chunk.textString());
                            break;
                        case EntryValue::hidl_discriminator::byteString:
                            EXPECT_EQ(value.byteString(), chunk.byteString());
                            break;
                        case EntryValue::hidl_discriminator::booleanValue:
                            EXPECT_EQ(value.booleanValue(), chunk.booleanValue());
                            break;
                    }
                });
            n++;
        }
        authenticatedDataBuilder.addDataItem(entry.nameSpace, entry.name, entry.value);
    }

    // Check the AuthenticatedData CBOR that we built during retrieval matches
    // exactly what we expect.
    ASSERT_TRUE(authenticatedDataBuilder.getEncodedCbor(encodedCbor));
    ASSERT_TRUE(support::cborPrettyPrint(encodedCbor, cborPretty, 32, {"EphemeralPublicKey"}));
    ASSERT_EQ(
        "{\n"
        "  'SessionTranscript' : {\n"
        "    'EphemeralPublicKey' : <not printed>\n"
        "  },\n"
        "  'Response' : {\n"
        "    'org.iso.18013-5.2019.mdl' : {\n"
        "      'PersonalData' : {\n"
        "        'Last name' : 'Turing',\n"
        "        'Birth date' : '19120623',\n"
        "        'First name' : 'Alan',\n"
        "        'Home address' : 'Maida Vale, London, England'\n"
        "      },\n"
        "      'Image' : {\n"
        "        'Portrait image' : <bstr size=262134 "
        "sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}",
        cborPretty);

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> generatedSigningKeyBlob;
    vector<uint8_t> generatedSigningKeyCertificate;
    credential->generateSigningKeyPair([&](ResultCode hidl_error,
                                           const hidl_vec<uint8_t> signingKeyBlob,
                                           const hidl_vec<uint8_t> signingKeyCertificate) {
        ASSERT_EQ(ResultCode::OK, hidl_error);
        generatedSigningKeyBlob = signingKeyBlob;
        generatedSigningKeyCertificate = signingKeyCertificate;
    });
    // TODO: check signingKeyCertificate is signed by the credential key.

    vector<uint8_t> retrievalPersonalizationReceipt;
    vector<uint8_t> previousAuditSignatureHash;
    credential->finishRetrieval(
        generatedSigningKeyBlob, previousAuditSignatureHash,
        [&](ResultCode hidl_error, const hidl_vec<uint8_t> personalizationReceipt,
            const AuditLogEntry& auditLogEntry) {
            ASSERT_EQ(ResultCode::OK, hidl_error);
            retrievalPersonalizationReceipt = personalizationReceipt;
            ASSERT_GT(personalizationReceipt.size(), 0);
        });
    EXPECT_TRUE(support::checkEcDsaSignature(support::sha256(encodedCbor),
                                             retrievalPersonalizationReceipt,
                                             generatedSigningKeyCertificate));
}

}  // namespace test
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::android::base::InitLogging(argv, &android::base::StderrLogger);
    int status = RUN_ALL_TESTS();
    return status;
}
