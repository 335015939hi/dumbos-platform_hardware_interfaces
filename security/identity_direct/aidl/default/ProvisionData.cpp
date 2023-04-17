/*
 * Copyright 2023 The Android Open Source Project
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
#include <android-base/logging.h>

#include "ProvisionData.h"

namespace aidl::android::security::identity::direct_access {

static const uint8_t testKey[AES_128_KEY_SIZE] = {0};
static const uint8_t realKey[AES_128_KEY_SIZE] = {0, 1, 2,  3,  4,  5,  6,  7,
                                                  8, 9, 10, 11, 12, 13, 14, 15};

ProvisionData::ProvisionData(bool in_testCredential, std::vector<uint8_t>& in_challenge,
                             uint32_t maxCredSize) {
    mTestCredential = in_testCredential;
    mCborCredentialData.resize(maxCredSize);
    mChallenge = in_challenge;
    mUsageCount = 0;
}

void ProvisionData::setCredentialKeys(std::vector<uint8_t>& priKey, std::vector<uint8_t>& pubKey) {
    memcpy(mCredentialPrivKey, priKey.data(), P256_PRIV_KEY_SIZE);
    memcpy(mCredentialPubKey, pubKey.data(), P256_PUB_KEY_SIZE);
}

std::optional<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>
ProvisionData::presentationPackageGenerate(uint64_t validityPeriodMillis, uint64_t currentTime) {
    unsigned int serial = 1;
    std::string serialDecimal = ::android::base::StringPrintf("%d", serial);
    std::map<std::string, std::vector<uint8_t>> extensions;
    const char* issuerName = "Android Identity Credential Key";                  // issuer CN
    const char* subjectName = "Android Identity Credential Authentication Key";  // subject CN
    // create Sigining keys
    std::optional<std::vector<uint8_t>> keyPair =
            ::android::hardware::identity::support::createEcKeyPair();
    if (!keyPair) {
        return std::nullopt;
    }
    std::optional<std::vector<uint8_t>> privKey =
            ::android::hardware::identity::support::ecKeyPairGetPrivateKey(keyPair.value());
    if (!privKey) {
        return std::nullopt;
    }
    if (privKey.value().size() != P256_PRIV_KEY_SIZE) {
        // ("Private key is %zd bytes, expected %zd", privKey.value().size(),
        //         (size_t) P256_PRIV_KEY_SIZE);
        return std::nullopt;
    }
    std::optional<std::vector<uint8_t>> pubKey =
            ::android::hardware::identity::support::ecKeyPairGetPublicKey(keyPair.value());
    if (!pubKey) {
        return std::nullopt;
    }
    if (pubKey.value().size() != P256_PUB_KEY_SIZE) {
        //("Public key is %zd bytes long, expected %zd", pubKey.value().size(),
        //         (size_t) P256_PRIV_KEY_SIZE + 1);
        return std::nullopt;
    }
    // convert variables in to Vec
    std::vector<uint8_t> credentialPrivKeyVec;
    credentialPrivKeyVec.resize(P256_PRIV_KEY_SIZE);
    memcpy(credentialPrivKeyVec.data(), mCredentialPrivKey, P256_PRIV_KEY_SIZE);
    std::vector<uint8_t> credentialPubKeyVec;
    credentialPubKeyVec.resize(P256_PUB_KEY_SIZE);
    memcpy(credentialPubKeyVec.data(), mCredentialPubKey, P256_PUB_KEY_SIZE);
    std::vector<uint8_t> storageKey;
    storageKey.resize(AES_128_KEY_SIZE);
    memcpy(storageKey.data(), mTestCredential ? testKey : realKey, AES_128_KEY_SIZE);
    // setting null data rather than passing parent null
    std::vector<uint8_t> nil;
    cppbor::Map credData = cppbor::Map()
                                   .add("docType", nil)
                                   .add("digestIdMapping", nil)
                                   .add("issuerAuth", nil)
                                   .add("readerAccess", nil);
    std::vector<uint8_t> signKeyAndCredData =
            cppbor::Array()
                    .add(cppbor::SemanticTag(24, privKey.value()))
                    .add(cppbor::SemanticTag(24, signKeyAndCredData))
                    .encode();

    std::optional<std::vector<uint8_t>> signingCertificate =
            ::android::hardware::identity::support::ecPublicKeyGenerateCertificate(
                    pubKey.value(), credentialPrivKeyVec, serialDecimal, issuerName, subjectName,
                    currentTime, validityPeriodMillis + currentTime, extensions);
    if (!signingCertificate) {
        return std::nullopt;
    }
    std::optional<std::vector<uint8_t>> nonce =
            ::android::hardware::identity::support::getRandom(NONCE_SIZE);
    if (!nonce.has_value()) {
        return std::nullopt;
    }
    std::optional<std::vector<uint8_t>> cppEncryptedData =
            ::android::hardware::identity::support::encryptAes128Gcm(
                    storageKey, nonce.value(), signKeyAndCredData, credentialPubKeyVec);
    if (!cppEncryptedData.has_value()) {
        return std::nullopt;
    }
    // save signing private key
    mPriSigningKey = privKey.value();
    return std::make_pair(signingCertificate.value(), cppEncryptedData.value());
}

std::optional<std::vector<uint8_t>> ProvisionData::getEncryptedCredData(
        const std::vector<uint8_t>& in_encryptedData, /* SigningKeyAndCredentialData */
        const std::vector<uint8_t>& in_credentialData /* CredentialData */) {
    // get storage key vec
    std::vector<uint8_t> storageKey;
    storageKey.resize(AES_128_KEY_SIZE);
    memcpy(storageKey.data(), mTestCredential ? testKey : realKey, AES_128_KEY_SIZE);
    // get Credential public key vec
    std::vector<uint8_t> credentialPubKeyVec;
    credentialPubKeyVec.resize(P256_PUB_KEY_SIZE);
    memcpy(credentialPubKeyVec.data(), mCredentialPubKey, P256_PUB_KEY_SIZE);

    std::optional<std::vector<uint8_t>> decryptedDataVec =
            ::android::hardware::identity::support::decryptAes128Gcm(storageKey, in_encryptedData,
                                                                     credentialPubKeyVec);
    if (!decryptedDataVec.has_value() ||
        decryptedDataVec.value().size() != in_encryptedData.size() - 28) {
        return std::nullopt;
    }

    // decrypt the "SigningKeyAndCredentialData"
    auto [item, _, message] = cppbor::parse(decryptedDataVec.value());
    if (item == nullptr) {
        return std::nullopt;
    }

    const cppbor::Array* arrayItem = item->asArray();
    if (arrayItem == nullptr || arrayItem->size() != 2) {
        return std::nullopt;
    }

    const cppbor::Bstr* privSigningKey = (*arrayItem)[0]->asBstr();
    const cppbor::Bstr* credentialData = (*arrayItem)[1]->asBstr();
    if (privSigningKey == nullptr || credentialData == nullptr) {
        return std::nullopt;
    }

    // encapsulate the code
    // TODO: need to verify the Credential Data CBOR
    std::vector<uint8_t> signKeyAndCredData =
            cppbor::Array()
                    .add(cppbor::SemanticTag(24, privSigningKey->value()))
                    .add(cppbor::SemanticTag(24, in_credentialData))
                    .encode();

    std::optional<std::vector<uint8_t>> nonce =
            ::android::hardware::identity::support::getRandom(NONCE_SIZE);
    if (!nonce.has_value()) {
        return std::nullopt;
    }

    std::optional<std::vector<uint8_t>> cppEncryptedData =
            ::android::hardware::identity::support::encryptAes128Gcm(
                    storageKey, nonce.value(), signKeyAndCredData, credentialPubKeyVec);
    if (!cppEncryptedData.has_value()) {
        return std::nullopt;
    }

    return cppEncryptedData;
}

bool ProvisionData::currentPresentationPackageSet(const std::vector<uint8_t>& in_signingCertificate,
                                                  const std::vector<uint8_t>& in_encryptedData) {
    // get storage key vec
    std::vector<uint8_t> storageKey;
    storageKey.resize(AES_128_KEY_SIZE);
    memcpy(storageKey.data(), mTestCredential ? testKey : realKey, AES_128_KEY_SIZE);
    // get Credential public key vec
    std::vector<uint8_t> credentialPubKeyVec;
    credentialPubKeyVec.resize(P256_PUB_KEY_SIZE);
    memcpy(credentialPubKeyVec.data(), mCredentialPubKey, P256_PUB_KEY_SIZE);

    std::optional<std::vector<uint8_t>> decryptedDataVec =
            ::android::hardware::identity::support::decryptAes128Gcm(storageKey, in_encryptedData,
                                                                     credentialPubKeyVec);
    if (!decryptedDataVec.has_value() ||
        decryptedDataVec.value().size() != in_encryptedData.size() - 28) {
        return false;
    }

    // decrypt the "SigningKeyAndCredentialData"
    auto [item, _, message] = cppbor::parse(decryptedDataVec.value());
    if (item == nullptr) {
        return false;
    }

    const cppbor::Array* arrayItem = item->asArray();
    if (arrayItem == nullptr || arrayItem->size() != 2) {
        return false;
    }

    const cppbor::Bstr* privSigningKey = (*arrayItem)[0]->asBstr();
    const cppbor::Bstr* credentialData = (*arrayItem)[1]->asBstr();
    if (privSigningKey == nullptr || credentialData == nullptr) {
        return false;
    }

    // assign the presention package variables
    mSigningKeyCertificate = in_signingCertificate;
    mPriSigningKey = privSigningKey->value();
    mCborCredentialData = credentialData->value();
    mUsageCount = 0;
    return true;
}

void ProvisionData::clearPresentationPackage() {
    mUsageCount = 0;
    /* mDocPresentationPackage reset */
    mSigningKeyCertificate.clear();
    mPriSigningKey.clear();
    mCborCredentialData.clear();
}

std::optional<std::vector<uint8_t>> ProvisionData::getCurrentSigningCertificate() {
    if (mSigningKeyCertificate.size() > 0) {
        return mSigningKeyCertificate;
    }

    return std::nullopt;
}

void ProvisionData::resetUsageCount() {
    mUsageCount = 0;
}

uint32_t ProvisionData::getUsageCount() {
    return mUsageCount;
}
}  // namespace aidl::android::security::identity::direct_access