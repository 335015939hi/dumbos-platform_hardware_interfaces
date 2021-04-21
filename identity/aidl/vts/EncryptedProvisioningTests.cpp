/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "EncryptedProvisioningTests"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <aidl/android/hardware/keymaster/VerificationToken.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
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
using ::android::base::StringPrintf;
using ::android::binder::Status;

using ::android::hardware::keymaster::HardwareAuthToken;
using ::android::hardware::keymaster::VerificationToken;

class EncryptedProvisioningTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
        halApiVersion_ = credentialStore_->getInterfaceVersion();
    }

    void provisionData();

    // Set by provisionData
    vector<uint8_t> credentialData_;
    vector<uint8_t> credentialPubKey_;

    SecureAccessControlProfile sacp_;
    vector<uint8_t> encContentStringValue_;
    vector<vector<uint8_t>> encContentBlobValues_;

    vector<uint8_t> cborStringValue_;
    vector<uint8_t> cborBlobValue_;

    sp<IIdentityCredentialStore> credentialStore_;
    int halApiVersion_;
};

pair<vector<vector<uint8_t>>, size_t> issuerDataEncrypt(const vector<uint8_t>& key,
                                                        const vector<uint8_t> aad,
                                                        const vector<uint8_t> data,
                                                        size_t chunkSize) {
    vector<vector<uint8_t>> encryptedChunks;
    size_t totalSize = 0;
    vector<vector<uint8_t>> chunks = support::chunkVector(data, chunkSize);
    for (const auto& chunk : chunks) {
        vector<uint8_t> nonce = support::getRandom(support::kAesGcmIvSize).value();
        vector<uint8_t> encrypted = support::encryptAes128Gcm(key, nonce, chunk, aad).value();
        encryptedChunks.push_back(encrypted);
        totalSize += encrypted.size();
    }
    return make_pair(encryptedChunks, totalSize);
}

void EncryptedProvisioningTests::provisionData() {
    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credentialStore_->createCredential(docType, testCredential, &wc).isOk());

    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    ASSERT_TRUE(wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                              &certChain)
                        .isOk());

    optional<vector<uint8_t>> optCredentialPubKey =
            support::certificateChainGetTopMostKey(certChain[0].encodedCertificate);
    ASSERT_TRUE(optCredentialPubKey);
    credentialPubKey_ = optCredentialPubKey.value();

    optional<vector<uint8_t>> issuerEphemeralKeyPair = support::createEcKeyPair();
    ASSERT_TRUE(issuerEphemeralKeyPair);
    optional<vector<uint8_t>> issuerEphemeralPublicKey =
            support::ecKeyPairGetPublicKey(issuerEphemeralKeyPair.value());
    ASSERT_TRUE(wc->setIssuerEphemeralPublicKey(issuerEphemeralPublicKey.value()).isOk());

    optional<vector<uint8_t>> issuerEphemeralPrivateKey =
            support::ecKeyPairGetPrivateKey(issuerEphemeralKeyPair.value());
    optional<vector<uint8_t>> sharedSecret =
            support::ecdh(credentialPubKey_, issuerEphemeralPrivateKey.value());
    vector<uint8_t> salt = {};  // TODO: maybe put entropy in salt
    string infoStr = "ICEncProv";
    vector<uint8_t> info = vector<uint8_t>(infoStr.begin(), infoStr.end());
    optional<vector<uint8_t>> dataEncKey = support::hkdf(sharedSecret.value(), salt, info, 16);

    size_t proofOfProvisioningSize = 262509;
    // Not in v1 HAL, may fail
    wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize);

    ASSERT_TRUE(wc->startPersonalization(1 /* numAccessControlProfiles */,
                                         {2} /* numDataElementsPerNamespace */)
                        .isOk());

    // Access control profile 0: open access
    ASSERT_TRUE(wc->addAccessControlProfile(0, {}, false, 0, 0, &sacp_).isOk());

    vector<uint8_t> dataEncAad = {};  // TODO

    // Entry which fits in a single chunk
    string stringValue = "Hello World";
    cborStringValue_ = cppbor::Tstr(stringValue).encode();
    auto [encStringChunks, encStringChunksTotalSize] = issuerDataEncrypt(
            dataEncKey.value(), dataEncAad, cborStringValue_, hwInfo.dataChunkSize);
    ASSERT_TRUE(
            wc->beginAddEncryptedEntry({0}, "ns", "String Value", encStringChunksTotalSize).isOk());
    ASSERT_EQ(1, encStringChunks.size());
    ASSERT_TRUE(wc->addEncryptedEntryValue(encStringChunks[0], &encContentStringValue_).isOk());

    // Entry which doesn't fit in a single chunk
    vector<uint8_t> blobValue;
    blobValue.resize(256 * 1024);
    for (size_t n = 0; n < blobValue.size(); n++) {
        blobValue[n] = uint8_t(n & 255);
    }
    cborBlobValue_ = cppbor::Bstr(blobValue).encode();
    auto [encBlobChunks, encBlobChunksTotalSize] =
            issuerDataEncrypt(dataEncKey.value(), dataEncAad, cborBlobValue_, hwInfo.dataChunkSize);
    ASSERT_TRUE(wc->beginAddEncryptedEntry({0}, "ns", "Blob Value", encBlobChunksTotalSize).isOk());
    ASSERT_LT(1, encBlobChunks.size());
    encContentBlobValues_ = {};
    vector<uint8_t> blobEncryptedValue;
    for (const auto& encChunk : encBlobChunks) {
        blobEncryptedValue.insert(blobEncryptedValue.end(), encChunk.begin(), encChunk.end());
        vector<uint8_t> encryptedChunk;
        ASSERT_TRUE(wc->addEncryptedEntryValue(encChunk, &encryptedChunk).isOk());
        encContentBlobValues_.push_back(encryptedChunk);
    }

    vector<uint8_t> proofOfProvisioningSignature;
    Status status = wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey_));

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty = cppbor::prettyPrint(proofOfProvisioning.value(), 32, {});
    EXPECT_EQ(StringPrintf("[\n"
                           "  'ProofOfProvisioning',\n"
                           "  'org.iso.18013-5.2019.mdl',\n"
                           "  [\n"
                           "    {\n"
                           "      'id' : 0,\n"
                           "    },\n"
                           "  ],\n"
                           "  {\n"
                           "    'ns' : [\n"
                           "      {\n"
                           "        'name' : 'String Value',\n"
                           "        'encryptedValue' : <bstr size=40 sha1=%s>,\n"
                           "        'accessControlProfiles' : [0, ],\n"
                           "      },\n"
                           "      {\n"
                           "        'name' : 'Blob Value',\n"
                           "        'encryptedValue' : <bstr size=262289 sha1=%s>,\n"
                           "        'accessControlProfiles' : [0, ],\n"
                           "      },\n"
                           "    ],\n"
                           "  },\n"
                           "  true,\n"
                           "]",
                           support::encodeHex(support::sha1(encStringChunks[0])).c_str(),
                           support::encodeHex(support::sha1(blobEncryptedValue)).c_str()),
              cborPretty);
}

// From ReaderAuthTest.cpp - TODO: consolidate with Util.h
RequestDataItem buildRequestDataItem(const string& name, size_t size,
                                     vector<int32_t> accessControlProfileIds);

TEST_P(EncryptedProvisioningTests, encryptedProvisioning) {
    if (halApiVersion_ < 3) { /* TODO: 4 instead of 3 */
        GTEST_SKIP() << "Need HAL API version 3, have " << halApiVersion_;
    }

    provisionData();

    sp<IIdentityCredential> credential;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &credential)
                        .isOk());

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
    // OK to fail, not available in v1 HAL
    credential->setVerificationToken(verificationToken);

    RequestNamespace rns;
    rns.namespaceName = "ns";
    rns.items.push_back(buildRequestDataItem("String Value", cborStringValue_.size(), {0}));
    rns.items.push_back(buildRequestDataItem("Blob Value", cborBlobValue_.size(), {0}));
    // OK to fail, not available in v1 HAL
    credential->setRequestedNamespaces({rns}).isOk();

    Status status = credential->startRetrieval({sacp_}, authToken, {},  // itemsRequestBytes
                                               {},                      // signingKeyBlob
                                               {},                      // sessionTranscriptBytes,
                                               {},                      // readerSignature
                                               {2 /* numDataElementsPerNamespace */});
    ASSERT_TRUE(status.isOk());

    // Check we can retrieve String Value (fits in a single chunk)
    ASSERT_TRUE(credential
                        ->startRetrieveEntryValue("ns", "String Value",
                                                  encContentStringValue_.size() - 28, {0})
                        .isOk());
    vector<uint8_t> decryptedCborStringValue;
    ASSERT_TRUE(credential->retrieveEntryValue(encContentStringValue_, &decryptedCborStringValue)
                        .isOk());
    ASSERT_EQ(decryptedCborStringValue, cborStringValue_);

    // Check we can retrieve String Value (does not fit in a single chunk)
    ASSERT_TRUE(credential->startRetrieveEntryValue("ns", "Blob Value", cborBlobValue_.size(), {0})
                        .isOk());
    vector<uint8_t> decryptedCborBlobValue;
    for (const auto& encBlob : encContentBlobValues_) {
        vector<uint8_t> decrypted;
        ASSERT_TRUE(credential->retrieveEntryValue(encBlob, &decrypted).isOk());
        decryptedCborBlobValue.insert(decryptedCborBlobValue.end(), decrypted.begin(),
                                      decrypted.end());
    }
    ASSERT_EQ(decryptedCborBlobValue, cborBlobValue_);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EncryptedProvisioningTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, EncryptedProvisioningTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
