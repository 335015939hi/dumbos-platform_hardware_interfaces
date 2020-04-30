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

#define LOG_TAG "VtsIWritableIdentityCredentialTests"

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
#include <utility>

#include "VtsIdentityTestUtils.h"

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

class ReaderAuthTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    void provisionData();
    void retrieveData(const vector<uint8_t>& readerPrivateKey,
                      const vector<vector<uint8_t>>& readerCertChain);

    // Set by provisionData
    vector<uint8_t> readerPublicKey_;
    vector<uint8_t> readerPrivateKey_;
    vector<uint8_t> rootAPublicKey_;
    vector<uint8_t> rootAPrivateKey_;
    vector<uint8_t> intermediateBPublicKey_;
    vector<uint8_t> intermediateBPrivateKey_;
    vector<uint8_t> rootCPublicKey_;
    vector<uint8_t> rootCPrivateKey_;

    vector<uint8_t> cert_A_;

    vector<uint8_t> cert_B_;

    vector<uint8_t> cert_C_;

    vector<uint8_t> cert_readerSelfsigned_;
    vector<uint8_t> cert_readerSignedByA_;
    vector<uint8_t> cert_readerSignedByB_;

    SecureAccessControlProfile sacp0_;
    SecureAccessControlProfile sacp1_;
    SecureAccessControlProfile sacp2_;

    vector<uint8_t> encContentAccessibleByA_;
    vector<uint8_t> encContentAccessibleByAorB_;
    vector<uint8_t> encContentAccessibleByB_;
    vector<uint8_t> encContentAccessibleByC_;

    vector<uint8_t> credentialData_;

    // Set by retrieveData()
    bool canGetAccessibleByA_;
    bool canGetAccessibleByAorB_;
    bool canGetAccessibleByB_;
    bool canGetAccessibleByC_;

    sp<IIdentityCredentialStore> credentialStore_;
};

pair<vector<uint8_t>, vector<uint8_t>> generateReaderKey() {
    optional<vector<uint8_t>> keyPKCS8 = support::createEcKeyPair();
    optional<vector<uint8_t>> publicKey = support::ecKeyPairGetPublicKey(keyPKCS8.value());
    optional<vector<uint8_t>> privateKey = support::ecKeyPairGetPrivateKey(keyPKCS8.value());
    return make_pair(publicKey.value(), privateKey.value());
}

vector<uint8_t> generateReaderCert(const vector<uint8_t>& publicKey,
                                   const vector<uint8_t>& signingKey) {
    time_t validityNotBefore = 0;
    time_t validityNotAfter = 0xffffffff;
    optional<vector<uint8_t>> cert =
            support::ecPublicKeyGenerateCertificate(publicKey, signingKey, "24601", "Issuer",
                                                    "Subject", validityNotBefore, validityNotAfter);
    return cert.value();
}

void ReaderAuthTests::provisionData() {
    // Keys and certificates for intermediates and root
    tie(rootAPublicKey_, rootAPrivateKey_) = generateReaderKey();
    tie(intermediateBPublicKey_, intermediateBPrivateKey_) = generateReaderKey();
    tie(rootCPublicKey_, rootCPrivateKey_) = generateReaderKey();

    cert_A_ = generateReaderCert(rootAPublicKey_, rootAPrivateKey_);

    cert_B_ = generateReaderCert(intermediateBPublicKey_, rootCPrivateKey_);

    cert_C_ = generateReaderCert(rootCPublicKey_, rootCPrivateKey_);

    // Key and self-signed certificate reader
    tie(readerPublicKey_, readerPrivateKey_) = generateReaderKey();
    cert_readerSelfsigned_ = generateReaderCert(readerPublicKey_, readerPrivateKey_);

    // Certificate for reader signed by intermediates
    cert_readerSignedByA_ = generateReaderCert(readerPublicKey_, rootAPrivateKey_);
    cert_readerSignedByB_ = generateReaderCert(readerPublicKey_, intermediateBPrivateKey_);

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credentialStore_->createCredential(docType, testCredential, &wc).isOk());

    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    ASSERT_TRUE(wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                              &certChain)
                        .isOk());

    size_t proofOfProvisioningSize = 350 + cert_A_.size() + cert_B_.size() + cert_C_.size();
    ASSERT_TRUE(wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize).isOk());

    ASSERT_TRUE(wc->startPersonalization(3 /* numAccessControlProfiles */,
                                         {4} /* numDataElementsPerNamespace */)
                        .isOk());

    // AIDL expects certificates wrapped in the Certificate type...
    Certificate cert_A;
    Certificate cert_B;
    Certificate cert_C;
    cert_A.encodedCertificate = cert_A_;
    cert_B.encodedCertificate = cert_B_;
    cert_C.encodedCertificate = cert_C_;

    // Access control profile 0: accessible by A
    ASSERT_TRUE(wc->addAccessControlProfile(0, cert_A, false, 0, 0, &sacp0_).isOk());

    // Access control profile 1: accessible by B
    ASSERT_TRUE(wc->addAccessControlProfile(1, cert_B, false, 0, 0, &sacp1_).isOk());

    // Access control profile 2: accessible by C
    ASSERT_TRUE(wc->addAccessControlProfile(2, cert_C, false, 0, 0, &sacp2_).isOk());

    // Data Element: "Accessible by A"
    ASSERT_TRUE(wc->beginAddEntry({0}, "ns", "Accessible by A", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByA_).isOk());

    // Data Element: "Accessible by A or B"
    ASSERT_TRUE(wc->beginAddEntry({0, 1}, "ns", "Accessible by A or B", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByAorB_).isOk());

    // Data Element: "Accessible by B"
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "Accessible by B", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByB_).isOk());

    // Data Element: "Accessible by C"
    ASSERT_TRUE(wc->beginAddEntry({2}, "ns", "Accessible by C", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByC_).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    ASSERT_TRUE(wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature).isOk());
}

TEST_P(ReaderAuthTests, presentingChain_SelfSigned) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_readerSelfsigned_});
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_FALSE(canGetAccessibleByAorB_);
    EXPECT_FALSE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
}

TEST_P(ReaderAuthTests, presentingChain_A) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_readerSignedByA_, cert_A_});
    EXPECT_TRUE(canGetAccessibleByA_);
    EXPECT_TRUE(canGetAccessibleByAorB_);
    EXPECT_FALSE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
}

TEST_P(ReaderAuthTests, presentingChain_B_and_C) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_readerSignedByB_, cert_B_, cert_C_});
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_TRUE(canGetAccessibleByAorB_);
    EXPECT_TRUE(canGetAccessibleByB_);
    EXPECT_TRUE(canGetAccessibleByC_);
}

RequestDataItem buildRequestDataItem(const string& name, size_t size,
                                     vector<int32_t> accessControlProfileIds) {
    RequestDataItem item;
    item.name = name;
    item.size = size;
    item.accessControlProfileIds = accessControlProfileIds;
    return item;
}

void ReaderAuthTests::retrieveData(const vector<uint8_t>& readerPrivateKey,
                                   const vector<vector<uint8_t>>& readerCertChain) {
    canGetAccessibleByA_ = false;
    canGetAccessibleByAorB_ = false;
    canGetAccessibleByB_ = false;
    canGetAccessibleByC_ = false;

    sp<IIdentityCredential> c;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &c)
                        .isOk());

    optional<vector<uint8_t>> readerEKeyPair = support::createEcKeyPair();
    optional<vector<uint8_t>> readerEPublicKey =
            support::ecKeyPairGetPublicKey(readerEKeyPair.value());
    ASSERT_TRUE(c->setReaderEphemeralPublicKey(readerEPublicKey.value()).isOk());

    vector<uint8_t> eKeyPair;
    ASSERT_TRUE(c->createEphemeralKeyPair(&eKeyPair).isOk());
    optional<vector<uint8_t>> ePublicKey = support::ecKeyPairGetPublicKey(eKeyPair);

    // Calculate requestData field and sign it with the reader key.
    auto [getXYSuccess, ephX, ephY] = support::ecPublicKeyGetXandY(ePublicKey.value());
    ASSERT_TRUE(getXYSuccess);
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", ephX).add("ephY", ephY);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    cppbor::Array sessionTranscript = cppbor::Array()
                                              .add(cppbor::Semantic(24, deviceEngagementBytes))
                                              .add(cppbor::Semantic(24, eReaderPubBytes));
    vector<uint8_t> sessionTranscriptBytes = sessionTranscript.encode();

    vector<uint8_t> itemsRequestBytes =
            cppbor::Map("nameSpaces",
                        cppbor::Map().add("ns", cppbor::Map()
                                                        .add("Accessible by A", false)
                                                        .add("Accessible by A or B", false)
                                                        .add("Accessible by B", false)
                                                        .add("Accessible by C", false)))
                    .encode();
    vector<uint8_t> dataToSign = cppbor::Array()
                                         .add("ReaderAuthentication")
                                         .add(sessionTranscript.clone())
                                         .add(cppbor::Semantic(24, itemsRequestBytes))
                                         .encode();

    optional<vector<uint8_t>> readerSignature =
            support::coseSignEcDsa(readerPrivateKey,  // private key for reader
                                   {},                // content
                                   dataToSign,        // detached content
                                   support::certificateChainJoin(readerCertChain));
    ASSERT_TRUE(readerSignature);

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(c->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());

    RequestNamespace rns;
    rns.namespaceName = "ns";
    rns.items.push_back(buildRequestDataItem("Accessible by A", 1, {0}));
    rns.items.push_back(buildRequestDataItem("Accessible by A or B", 1, {0, 1}));
    rns.items.push_back(buildRequestDataItem("Accessible by B", 1, {1}));
    rns.items.push_back(buildRequestDataItem("Accessible by C", 1, {2}));
    ASSERT_TRUE(c->setRequestedNamespaces({rns}).isOk());

    HardwareAuthToken authToken;
    authToken.userId = 0;
    ASSERT_TRUE(c->startRetrieval({sacp0_, sacp1_, sacp2_}, authToken, itemsRequestBytes,
                                  signingKeyBlob, sessionTranscriptBytes, readerSignature.value(),
                                  {4})
                        .isOk());

    Status status;
    vector<uint8_t> decrypted;

    status = c->startRetrieveEntryValue("ns", "Accessible by A", 1, {0});
    if (status.isOk()) {
        canGetAccessibleByA_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByA_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by A or B", 1, {0, 1});
    if (status.isOk()) {
        canGetAccessibleByAorB_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByAorB_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by B", 1, {1});
    if (status.isOk()) {
        canGetAccessibleByB_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByB_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by C", 1, {2});
    if (status.isOk()) {
        canGetAccessibleByC_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByC_, &decrypted).isOk());
    }

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpaces;
    ASSERT_TRUE(c->finishRetrieval(&mac, &deviceNameSpaces).isOk());
}

INSTANTIATE_TEST_SUITE_P(
        Identity, ReaderAuthTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
