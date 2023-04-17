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
#include <aidl/android/security/identity/direct_access/BnMDocStore.h>

#include <android-base/logging.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include "ProvisionData.h"

#include "FakeSecureElementProxy.h"

constexpr int32_t kMaxCredentialSlots = 1;
constexpr int64_t kMaxCredentialDataSize = 32768;

namespace aidl::android::security::identity::direct_access {

FakeSecureElementProxy::FakeSecureElementProxy() {
    mProvisionData.resize(kMaxCredentialSlots, nullptr);
}

int32_t FakeSecureElementProxy::createCredential(int32_t credentialSlot, bool testCredential,
                                                 std::vector<uint8_t> challenge,
                                                 std::vector<Certificate>* certificate) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }
    if (mProvisionData[credentialSlot] != nullptr) {
        return IMDocStore::STATUS_CREDENTIAL_ALREADY_EXISTS;
    }
    std::vector<uint8_t> keyPair;
    std::vector<uint8_t> certs;
    std::vector<uint8_t> applicationId({1});  // TODO

    std::optional<std::pair<std::vector<uint8_t>, std::vector<std::vector<uint8_t>>>> keyCert =
            ::android::hardware::identity::support::createEcKeyPairAndAttestation(
                    challenge, applicationId, testCredential);
    if (!keyCert) {
        return IMDocStore::STATUS_FAILED;
    }
    keyPair = std::move(keyCert->first);
    certs = ::android::hardware::identity::support::certificateChainJoin(keyCert->second);

    std::optional<std::vector<std::vector<uint8_t>>> retCert =
            ::android::hardware::identity::support::certificateChainSplit(certs);
    if (!retCert) {
        return IMDocStore::STATUS_FAILED;
    }

    // Extract private/public key.
    std::optional<std::vector<uint8_t>> privKey =
            ::android::hardware::identity::support::ecKeyPairGetPrivateKey(keyPair);
    std::optional<std::vector<uint8_t>> pubKey =
            ::android::hardware::identity::support::ecKeyPairGetPublicKey(keyPair);
    if (!privKey || privKey.value().size() != P256_PRIV_KEY_SIZE || !pubKey ||
        pubKey.value().size() != P256_PUB_KEY_SIZE) {
        return IMDocStore::STATUS_FAILED;
    }

    mProvisionData[credentialSlot] =
            std::make_shared<ProvisionData>(testCredential, challenge, kMaxCredentialDataSize);
    mProvisionData[credentialSlot]->setCredentialKeys(privKey.value(), pubKey.value());

    for (std::vector<uint8_t>& cert : retCert.value()) {
        Certificate c;
        c.encodedCertificate = std::move(cert);
        certificate->push_back(std::move(c));
    }
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::deleteCredential(int credentialSlot) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }
    if (mProvisionData[credentialSlot] != nullptr) {
        mProvisionData[credentialSlot].reset();
    }
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::getMaximumCredentialDataSize(int64_t* maxCredDataSize) {
    *maxCredDataSize = kMaxCredentialDataSize;
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::getNumberOfCredentialSlots(int32_t* slots) {
    *slots = kMaxCredentialSlots;
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::lookupMDocCredential(int credentialSlot) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }
    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::presentationPackageGenerate(
        uint32_t credentialSlot, uint64_t validityPeriodMillis, uint64_t currentTime,
        std::pair<std::vector<uint8_t>, std::vector<uint8_t>>* out) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    auto ret = mProvisionData[credentialSlot]->presentationPackageGenerate(validityPeriodMillis,
                                                                           currentTime);
    if (!ret) {
        return IMDocStore::STATUS_FAILED;
    }
    // TODO out = {std::move(), std::move()}
    out->first = std::move(ret->first);
    out->second = std::move(ret->second);
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::getEncryptedCredData(uint32_t credentialSlot,
                                                     const std::vector<uint8_t>& in_encryptedData,
                                                     const std::vector<uint8_t>& in_credentialData,
                                                     std::vector<uint8_t>* out) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    auto ret = mProvisionData[credentialSlot]->getEncryptedCredData(in_encryptedData,
                                                                    in_credentialData);
    if (!ret) {
        return IMDocStore::STATUS_FAILED;
    }
    *out = std::move(*ret);
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::currentPresentationPackageSet(
        uint32_t credentialSlot, const std::vector<uint8_t>& in_signingCertificate,
        const std::vector<uint8_t>& in_encryptedData) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    if (!mProvisionData[credentialSlot]->currentPresentationPackageSet(in_signingCertificate,
                                                                       in_encryptedData)) {
        return IMDocStore::STATUS_FAILED;
    }
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::getUsageCount(int credentialSlot, int32_t* out) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    *out = mProvisionData[credentialSlot]->getUsageCount();
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::getCurrentSigningCertificate(uint32_t credentialSlot,
                                                             std::vector<uint8_t>* out) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    auto ret = mProvisionData[credentialSlot]->getCurrentSigningCertificate();
    if (!ret) {
        return IMDocStore::STATUS_FAILED;
    }
    *out = std::move(*ret);
    return IMDocStore::STATUS_OK;
}

int32_t FakeSecureElementProxy::clearPresentationPackage(uint32_t credentialSlot) {
    if ((credentialSlot < 0) || credentialSlot > (kMaxCredentialSlots - 1)) {
        return IMDocStore::STATUS_FAILED;
    }

    if (mProvisionData[credentialSlot] == nullptr) {
        return IMDocStore::STATUS_NO_SUCH_CREDENTIAL;
    }
    mProvisionData[credentialSlot]->clearPresentationPackage();
    return IMDocStore::STATUS_OK;
}
}  // namespace aidl::android::security::identity::direct_access