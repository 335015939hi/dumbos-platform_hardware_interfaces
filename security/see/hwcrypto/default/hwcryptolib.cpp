/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <aidl/android/hardware/security/see/hwcrypto/BnCryptoOperationContext.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnHwCryptoOperations.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnOpaqueKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/IOpaqueKey.h>
#include <android-base/logging.h>
#include <android/hardware/security/see/hwcrypto/BnHwCryptoKey.h>
#include <binder/RpcTrusty.h>
#include <trusty/tipc.h>
#include <optional>
#include <string>
#include "hwcryptokeyimpl.h"

using android::IBinder;
using android::IInterface;
using android::RpcSession;
using android::RpcTrustyConnectWithSessionInitializer;
using android::sp;
using android::wp;
using android::base::ErrnoError;
using android::base::Error;
using android::base::Result;
using android::binder::Status;

namespace android {
namespace trusty {
namespace hwcryptohalservice {

#define HWCRYPTO_KEY_PORT "com.android.trusty.rust.hwcryptohal.V1"

// Even though we get the cpp_hwcrypto::IOpaqueKey and cpp_hwcrypto::ICryptoOperationContext and
// create the ndk_hwcrypto wrappers on this library we cannot cast them back when we need them
// because they are received on the function calls as binder objects and there is no reliable
// we to do this cast yet. Because of that we are creating maps to hold the wrapped objects
// and translate them on function calls.
// TODO: Add cleanup of both keyMapping and contextMapping once we have more test infrastructure in
//       place.
std::map<std::weak_ptr<ndk_hwcrypto::IOpaqueKey>, wp<cpp_hwcrypto::IOpaqueKey>, std::owner_less<>>
        keyMapping;
std::map<std::weak_ptr<ndk_hwcrypto::ICryptoOperationContext>,
         wp<cpp_hwcrypto::ICryptoOperationContext>, std::owner_less<>>
        contextMapping;

sp<cpp_hwcrypto::IOpaqueKey> convertKeyPointer(
        const std::shared_ptr<ndk_hwcrypto::IOpaqueKey>& opaqueKey) {
    if (opaqueKey == nullptr) {
        return nullptr;
    }
    if (keyMapping.find(opaqueKey) == keyMapping.end()) {
        LOG(ERROR) << "couldn't find wrapped key";
        return nullptr;
    }
    auto opaqueKeyCpp = keyMapping[opaqueKey];
    return opaqueKeyCpp.promote();
}

sp<cpp_hwcrypto::ICryptoOperationContext> convertOperationsContext(
        const std::shared_ptr<ndk_hwcrypto::ICryptoOperationContext>& context) {
    if (context == nullptr) {
        return nullptr;
    }
    if (contextMapping.find(context) == contextMapping.end()) {
        LOG(ERROR) << "couldn't find wrapped key";
        return nullptr;
    }
    auto contextCpp = contextMapping[context];
    return contextCpp.promote();
}

// TODO: Fix this function to also include the service specific exceptions
static ndk::ScopedAStatus convertStatus(Status status) {
    if (status.isOk()) {
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(status.exceptionCode(),
                                                                status.exceptionMessage());
    }
}

static std::optional<cpp_hwcrypto::types::ExplicitKeyMaterial> convertExplicitKeyMaterial(
        const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial) {
    auto explicitKeyCpp = cpp_hwcrypto::types::ExplicitKeyMaterial();

    if (keyMaterial.getTag() == ndk_hwcrypto::types::ExplicitKeyMaterial::aes) {
        auto aesKey = keyMaterial.get<ndk_hwcrypto::types::ExplicitKeyMaterial::aes>();
        auto aesKeyCpp = cpp_hwcrypto::types::AesKey();
        if (aesKey.getTag() == ndk_hwcrypto::types::AesKey::aes128) {
            aesKeyCpp.set<cpp_hwcrypto::types::AesKey::aes128>(
                    aesKey.get<ndk_hwcrypto::types::AesKey::aes128>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::aes>(aesKeyCpp);
        } else if (aesKey.getTag() == ndk_hwcrypto::types::AesKey::aes256) {
            aesKeyCpp.set<cpp_hwcrypto::types::AesKey::aes256>(
                    aesKey.get<ndk_hwcrypto::types::AesKey::aes256>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::aes>(aesKeyCpp);
        } else {
            LOG(ERROR) << "unknown AesKey type";
            return std::nullopt;
        }
    } else if (keyMaterial.getTag() == ndk_hwcrypto::types::ExplicitKeyMaterial::hmac) {
        auto hmacKey = keyMaterial.get<ndk_hwcrypto::types::ExplicitKeyMaterial::hmac>();
        auto hmacKeyCpp = cpp_hwcrypto::types::HmacKey();
        if (hmacKey.getTag() == ndk_hwcrypto::types::HmacKey::sha256) {
            hmacKeyCpp.set<cpp_hwcrypto::types::HmacKey::sha256>(
                    hmacKey.get<ndk_hwcrypto::types::HmacKey::sha256>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::hmac>(hmacKeyCpp);
        } else if (hmacKey.getTag() == ndk_hwcrypto::types::HmacKey::sha512) {
            hmacKeyCpp.set<cpp_hwcrypto::types::HmacKey::sha512>(
                    hmacKey.get<ndk_hwcrypto::types::HmacKey::sha512>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::hmac>(hmacKeyCpp);
        } else {
            LOG(ERROR) << "unknown HmacKey type";
            return std::nullopt;
        }
    } else {
        LOG(ERROR) << "unknown Key type";
        return std::nullopt;
    }
    return explicitKeyCpp;
}

std::optional<cpp_hwcrypto::KeyPolicy> convertKeyPolicy(
        const ndk_hwcrypto::KeyPolicy& ndkKeyPolicy) {
    cpp_hwcrypto::KeyPolicy policy = cpp_hwcrypto::KeyPolicy();
    switch (ndkKeyPolicy.usage) {
        case ndk_hwcrypto::types::KeyUse::ENCRYPT:
            policy.usage = cpp_hwcrypto::types::KeyUse::ENCRYPT;
            break;
        case ndk_hwcrypto::types::KeyUse::DECRYPT:
            policy.usage = cpp_hwcrypto::types::KeyUse::DECRYPT;
            break;
        case ndk_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT:
            policy.usage = cpp_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT;
            break;
        case ndk_hwcrypto::types::KeyUse::SIGN:
            policy.usage = cpp_hwcrypto::types::KeyUse::SIGN;
            break;
        case ndk_hwcrypto::types::KeyUse::DERIVE:
            policy.usage = cpp_hwcrypto::types::KeyUse::DERIVE;
            break;
        case ndk_hwcrypto::types::KeyUse::WRAP:
            policy.usage = cpp_hwcrypto::types::KeyUse::WRAP;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy usage";
            return std::nullopt;
    }

    switch (ndkKeyPolicy.keyLifetime) {
        case ndk_hwcrypto::types::KeyLifetime::EPHEMERAL:
            policy.keyLifetime = cpp_hwcrypto::types::KeyLifetime::EPHEMERAL;
            break;
        case ndk_hwcrypto::types::KeyLifetime::HARDWARE:
            policy.keyLifetime = cpp_hwcrypto::types::KeyLifetime::HARDWARE;
            break;
        case ndk_hwcrypto::types::KeyLifetime::PORTABLE:
            policy.keyLifetime = cpp_hwcrypto::types::KeyLifetime::PORTABLE;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy lifetime";
            return std::nullopt;
    }

    switch (ndkKeyPolicy.keyType) {
        case ndk_hwcrypto::types::KeyType::AES_128_CBC_NO_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_128_CBC_NO_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::AES_128_GCM:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_128_GCM;
            break;
        case ndk_hwcrypto::types::KeyType::AES_128_CMAC:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_128_CMAC;
            break;
        case ndk_hwcrypto::types::KeyType::AES_256_CBC_NO_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_256_CBC_NO_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::AES_256_CBC_PKCS7_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_256_CBC_PKCS7_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::AES_256_CTR:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_256_CTR;
            break;
        case ndk_hwcrypto::types::KeyType::AES_256_GCM:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_256_GCM;
            break;
        case ndk_hwcrypto::types::KeyType::AES_256_CMAC:
            policy.keyType = cpp_hwcrypto::types::KeyType::AES_256_CMAC;
            break;
        case ndk_hwcrypto::types::KeyType::HMAC_SHA256:
            policy.keyType = cpp_hwcrypto::types::KeyType::HMAC_SHA256;
            break;
        case ndk_hwcrypto::types::KeyType::HMAC_SHA512:
            policy.keyType = cpp_hwcrypto::types::KeyType::HMAC_SHA512;
            break;
        case ndk_hwcrypto::types::KeyType::RSA2048_PSS_SHA256:
            policy.keyType = cpp_hwcrypto::types::KeyType::RSA2048_PSS_SHA256;
            break;
        case ndk_hwcrypto::types::KeyType::RSA2048_PKCS1_5_SHA256:
            policy.keyType = cpp_hwcrypto::types::KeyType::RSA2048_PKCS1_5_SHA256;
            break;
        case ndk_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_NO_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_NO_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_SHA256:
            policy.keyType = cpp_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_SHA256;
            break;
        case ndk_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_NO_PADDING:
            policy.keyType = cpp_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_NO_PADDING;
            break;
        case ndk_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_SHA512:
            policy.keyType = cpp_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_SHA512;
            break;
        case ndk_hwcrypto::types::KeyType::ECC_ED25519_SIGN:
            policy.keyType = cpp_hwcrypto::types::KeyType::ECC_ED25519_SIGN;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy key type";
            return std::nullopt;
    }

    policy.keyManagementKey = ndkKeyPolicy.keyManagementKey;
    // TODO: Add key permissions translation
    return policy;
}

std::optional<const ndk_hwcrypto::KeyPolicy> convertKeyPolicy(
        cpp_hwcrypto::KeyPolicy& cppKeyPolicy) {
    ndk_hwcrypto::KeyPolicy policy = ndk_hwcrypto::KeyPolicy();
    switch (cppKeyPolicy.usage) {
        case cpp_hwcrypto::types::KeyUse::ENCRYPT:
            policy.usage = ndk_hwcrypto::types::KeyUse::ENCRYPT;
            break;
        case cpp_hwcrypto::types::KeyUse::DECRYPT:
            policy.usage = ndk_hwcrypto::types::KeyUse::DECRYPT;
            break;
        case cpp_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT:
            policy.usage = ndk_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT;
            break;
        case cpp_hwcrypto::types::KeyUse::SIGN:
            policy.usage = ndk_hwcrypto::types::KeyUse::SIGN;
            break;
        case cpp_hwcrypto::types::KeyUse::DERIVE:
            policy.usage = ndk_hwcrypto::types::KeyUse::DERIVE;
            break;
        case cpp_hwcrypto::types::KeyUse::WRAP:
            policy.usage = ndk_hwcrypto::types::KeyUse::WRAP;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy usage";
            return std::nullopt;
    }

    switch (cppKeyPolicy.keyLifetime) {
        case cpp_hwcrypto::types::KeyLifetime::EPHEMERAL:
            policy.keyLifetime = ndk_hwcrypto::types::KeyLifetime::EPHEMERAL;
            break;
        case cpp_hwcrypto::types::KeyLifetime::HARDWARE:
            policy.keyLifetime = ndk_hwcrypto::types::KeyLifetime::HARDWARE;
            break;
        case cpp_hwcrypto::types::KeyLifetime::PORTABLE:
            policy.keyLifetime = ndk_hwcrypto::types::KeyLifetime::PORTABLE;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy lifetime";
            return std::nullopt;
    }

    switch (cppKeyPolicy.keyType) {
        case cpp_hwcrypto::types::KeyType::AES_128_CBC_NO_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_128_CBC_NO_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::AES_128_GCM:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_128_GCM;
            break;
        case cpp_hwcrypto::types::KeyType::AES_128_CMAC:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_128_CMAC;
            break;
        case cpp_hwcrypto::types::KeyType::AES_256_CBC_NO_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_256_CBC_NO_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::AES_256_CBC_PKCS7_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_256_CBC_PKCS7_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::AES_256_CTR:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_256_CTR;
            break;
        case cpp_hwcrypto::types::KeyType::AES_256_GCM:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_256_GCM;
            break;
        case cpp_hwcrypto::types::KeyType::AES_256_CMAC:
            policy.keyType = ndk_hwcrypto::types::KeyType::AES_256_CMAC;
            break;
        case cpp_hwcrypto::types::KeyType::HMAC_SHA256:
            policy.keyType = ndk_hwcrypto::types::KeyType::HMAC_SHA256;
            break;
        case cpp_hwcrypto::types::KeyType::HMAC_SHA512:
            policy.keyType = ndk_hwcrypto::types::KeyType::HMAC_SHA512;
            break;
        case cpp_hwcrypto::types::KeyType::RSA2048_PSS_SHA256:
            policy.keyType = ndk_hwcrypto::types::KeyType::RSA2048_PSS_SHA256;
            break;
        case cpp_hwcrypto::types::KeyType::RSA2048_PKCS1_5_SHA256:
            policy.keyType = ndk_hwcrypto::types::KeyType::RSA2048_PKCS1_5_SHA256;
            break;
        case cpp_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_NO_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_NO_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_SHA256:
            policy.keyType = ndk_hwcrypto::types::KeyType::ECC_NIST_P256_SIGN_SHA256;
            break;
        case cpp_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_NO_PADDING:
            policy.keyType = ndk_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_NO_PADDING;
            break;
        case cpp_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_SHA512:
            policy.keyType = ndk_hwcrypto::types::KeyType::ECC_NIST_P521_SIGN_SHA512;
            break;
        case cpp_hwcrypto::types::KeyType::ECC_ED25519_SIGN:
            policy.keyType = ndk_hwcrypto::types::KeyType::ECC_ED25519_SIGN;
            break;
        default:
            // Enums do not have extra values so this should not happen
            LOG(ERROR) << "unknown value in key policy key type";
            return std::nullopt;
    }

    policy.keyManagementKey = cppKeyPolicy.keyManagementKey;
    // TODO: Add key permissions translation
    return policy;
}

class HwCryptoOperationContextNdk : public ndk_hwcrypto::BnCryptoOperationContext {
  private:
    sp<cpp_hwcrypto::ICryptoOperationContext> mContext;

  public:
    HwCryptoOperationContextNdk(sp<cpp_hwcrypto::ICryptoOperationContext> operations)
        : mContext(std::move(operations)) {}

    static std::shared_ptr<HwCryptoOperationContextNdk> Create(
            sp<cpp_hwcrypto::ICryptoOperationContext> operations) {
        if (operations == nullptr) {
            return nullptr;
        }
        std::shared_ptr<HwCryptoOperationContextNdk> contextNdk =
                ndk::SharedRefBase::make<HwCryptoOperationContextNdk>(std::move(operations));

        if (!contextNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoOperationContext";
            return nullptr;
        }
        return contextNdk;
    }
};

void insertWrappedContextIntoMap(
        sp<cpp_hwcrypto::ICryptoOperationContext> cppContext,
        std::shared_ptr<ndk_hwcrypto::ICryptoOperationContext>* ndkContext) {
    std::shared_ptr<ndk_hwcrypto::ICryptoOperationContext> operationsContext =
            HwCryptoOperationContextNdk::Create(cppContext);
    std::weak_ptr<ndk_hwcrypto::ICryptoOperationContext> weakContextNdk = operationsContext;
    wp<cpp_hwcrypto::ICryptoOperationContext> weakContextCpp = cppContext;
    contextMapping.insert({weakContextNdk, weakContextCpp});
    *ndkContext = operationsContext;
}

std::optional<cpp_hwcrypto::types::OperationData> convertOperationData(
        const ndk_hwcrypto::types::OperationData& ndkOperationData) {
    cpp_hwcrypto::types::OperationData cppOperationData = cpp_hwcrypto::types::OperationData();
    cpp_hwcrypto::types::MemoryBufferReference cppMemBuffRef;
    switch (ndkOperationData.getTag()) {
        case ndk_hwcrypto::types::OperationData::dataBuffer:
            cppOperationData.set<cpp_hwcrypto::types::OperationData::dataBuffer>(
                    ndkOperationData.get<ndk_hwcrypto::types::OperationData::dataBuffer>());
            break;
        case ndk_hwcrypto::types::OperationData::memoryBufferReference:
            cppMemBuffRef.startOffset =
                    ndkOperationData
                            .get<ndk_hwcrypto::types::OperationData::memoryBufferReference>()
                            .startOffset;
            cppMemBuffRef.sizeBytes =
                    ndkOperationData
                            .get<ndk_hwcrypto::types::OperationData::memoryBufferReference>()
                            .sizeBytes;
            cppOperationData.set<cpp_hwcrypto::types::OperationData::memoryBufferReference>(
                    std::move(cppMemBuffRef));
            break;
        default:
            LOG(ERROR) << "received unknown operation data type";
            return std::nullopt;
    }
    return cppOperationData;
}

std::optional<cpp_hwcrypto::PatternParameters> convertPatternParameters(
        const ndk_hwcrypto::PatternParameters& ndkpatternParameters) {
    int64_t numberBlocksProcess = ndkpatternParameters.numberBlocksProcess;
    int64_t numberBlocksCopy = ndkpatternParameters.numberBlocksCopy;
    if ((numberBlocksProcess < 0) || (numberBlocksCopy < 0)) {
        LOG(ERROR) << "received invalid pattern parameters";
        return std::nullopt;
    }
    cpp_hwcrypto::PatternParameters patternParameters = cpp_hwcrypto::PatternParameters();
    patternParameters.numberBlocksProcess = numberBlocksProcess;
    patternParameters.numberBlocksCopy = numberBlocksCopy;
    return patternParameters;
}

std::optional<cpp_hwcrypto::types::SymmetricOperation> convertSymmetricOperation(
        const ndk_hwcrypto::types::SymmetricOperation& ndkSymmetricOperation) {
    cpp_hwcrypto::types::SymmetricOperation symmetricOperation =
            cpp_hwcrypto::types::SymmetricOperation();
    switch (ndkSymmetricOperation) {
        case ndk_hwcrypto::types::SymmetricOperation::ENCRYPT:
            symmetricOperation = cpp_hwcrypto::types::SymmetricOperation::ENCRYPT;
            break;
        case ndk_hwcrypto::types::SymmetricOperation::DECRYPT:
            symmetricOperation = cpp_hwcrypto::types::SymmetricOperation::DECRYPT;
            break;
        default:
            LOG(ERROR) << "invalid symmetric operation type";
            return std::nullopt;
    }
    return symmetricOperation;
}

cpp_hwcrypto::types::CipherModeParameters convertSymmetricModeParameters(
        const ndk_hwcrypto::types::CipherModeParameters& ndkcipherModeParameters) {
    cpp_hwcrypto::types::CipherModeParameters cipherModeParameters =
            cpp_hwcrypto::types::CipherModeParameters();
    cipherModeParameters.nonce = ndkcipherModeParameters.nonce;
    return cipherModeParameters;
}

cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters convertSymmetricModeParameters(
        const ndk_hwcrypto::types::AesGcmMode::AesGcmModeParameters& ndkgcmModeParameters) {
    cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters gcmModeParameters =
            cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters();
    gcmModeParameters.nonce = ndkgcmModeParameters.nonce;
    return gcmModeParameters;
}

std::optional<cpp_hwcrypto::OperationParameters> convertOperationParameters(
        const ndk_hwcrypto::OperationParameters& ndkOperationParameters) {
    cpp_hwcrypto::OperationParameters operationParameters = cpp_hwcrypto::OperationParameters();
    sp<cpp_hwcrypto::IOpaqueKey> opaqueKey;
    cpp_hwcrypto::types::HmacOperationParameters hmacParameters =
            cpp_hwcrypto::types::HmacOperationParameters();
    std::optional<cpp_hwcrypto::types::SymmetricOperation> cppSymmetricOperation;
    cpp_hwcrypto::types::CipherModeParameters cipherModeParameters;
    cpp_hwcrypto::types::AesCipherMode cppAesCipherMode = cpp_hwcrypto::types::AesCipherMode();
    cpp_hwcrypto::types::SymmetricOperationParameters cppSymmetricOperationParameters =
            cpp_hwcrypto::types::SymmetricOperationParameters();
    cpp_hwcrypto::types::SymmetricAuthOperationParameters cppSymmetricAuthOperationParameters =
            cpp_hwcrypto::types::SymmetricAuthOperationParameters();
    cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters cppAesGcmModeParameters =
            cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters();
    cpp_hwcrypto::types::AesGcmMode cppAesGcmMode = cpp_hwcrypto::types::AesGcmMode();
    switch (ndkOperationParameters.getTag()) {
        case ndk_hwcrypto::OperationParameters::symmetricAuthCrypto:
            opaqueKey = convertKeyPointer(
                    ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get aes key";
                return std::nullopt;
            }
            cppSymmetricAuthOperationParameters.key = std::move(opaqueKey);
            cppSymmetricOperation = convertSymmetricOperation(
                    ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .direction);
            if (!cppSymmetricOperation.has_value()) {
                LOG(ERROR) << "couldn't get aes direction";
                return std::nullopt;
            }
            cppSymmetricAuthOperationParameters.direction =
                    std::move(cppSymmetricOperation.value());
            switch (ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .parameters.getTag()) {
                case ndk_hwcrypto::types::SymmetricAuthCryptoParameters::aes:
                    switch (ndkOperationParameters
                                    .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                                    .parameters
                                    .get<ndk_hwcrypto::types::SymmetricAuthCryptoParameters::aes>()
                                    .getTag()) {
                        case ndk_hwcrypto::types::AesGcmMode::gcmTag16:
                            cppAesGcmModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricAuthCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::
                                                         SymmetricAuthCryptoParameters::aes>()
                                            .get<ndk_hwcrypto::types::AesGcmMode::gcmTag16>());
                            cppAesGcmMode.set<cpp_hwcrypto::types::AesGcmMode::gcmTag16>(
                                    std::move(cppAesGcmModeParameters));
                            cppSymmetricAuthOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricAuthCryptoParameters::aes>(
                                            std::move(cppAesGcmMode));
                            break;
                        default:
                            LOG(ERROR) << "received invalid aes gcm parameters";
                            return std::nullopt;
                    }
                    break;
                default:
                    LOG(ERROR) << "received invalid symmetric auth crypto parameters";
                    return std::nullopt;
            }
            operationParameters.set<cpp_hwcrypto::OperationParameters::symmetricAuthCrypto>(
                    std::move(cppSymmetricAuthOperationParameters));
            break;
        case ndk_hwcrypto::OperationParameters::symmetricCrypto:
            opaqueKey = convertKeyPointer(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get aes key";
                return std::nullopt;
            }
            cppSymmetricOperationParameters.key = std::move(opaqueKey);
            cppSymmetricOperation = convertSymmetricOperation(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .direction);
            if (!cppSymmetricOperation.has_value()) {
                LOG(ERROR) << "couldn't get aes direction";
                return std::nullopt;
            }
            cppSymmetricOperationParameters.direction = std::move(cppSymmetricOperation.value());
            switch (ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .parameters.getTag()) {
                case ndk_hwcrypto::types::SymmetricCryptoParameters::aes:
                    switch (ndkOperationParameters
                                    .get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                                    .parameters
                                    .get<ndk_hwcrypto::types::SymmetricCryptoParameters::aes>()
                                    .getTag()) {
                        case ndk_hwcrypto::types::AesCipherMode::cbc:
                            cipherModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::SymmetricCryptoParameters::
                                                         aes>()
                                            .get<ndk_hwcrypto::types::AesCipherMode::cbc>());
                            cppAesCipherMode.set<cpp_hwcrypto::types::AesCipherMode::cbc>(
                                    std::move(cipherModeParameters));
                            cppSymmetricOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricCryptoParameters::aes>(
                                            std::move(cppAesCipherMode));
                            break;
                        case ndk_hwcrypto::types::AesCipherMode::ctr:
                            cipherModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::SymmetricCryptoParameters::
                                                         aes>()
                                            .get<ndk_hwcrypto::types::AesCipherMode::ctr>());
                            cppAesCipherMode.set<cpp_hwcrypto::types::AesCipherMode::ctr>(
                                    std::move(cipherModeParameters));
                            cppSymmetricOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricCryptoParameters::aes>(
                                            std::move(cppAesCipherMode));
                            break;
                        default:
                            LOG(ERROR) << "received invalid aes parameters";
                            return std::nullopt;
                    }
                    break;
                default:
                    LOG(ERROR) << "received invalid symmetric crypto parameters";
                    return std::nullopt;
            }
            operationParameters.set<cpp_hwcrypto::OperationParameters::symmetricCrypto>(
                    std::move(cppSymmetricOperationParameters));
            break;
        case ndk_hwcrypto::OperationParameters::hmac:
            opaqueKey = convertKeyPointer(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::hmac>().key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get hmac key";
                return std::nullopt;
            }
            hmacParameters.key = opaqueKey;
            operationParameters.set<cpp_hwcrypto::OperationParameters::hmac>(
                    std::move(hmacParameters));
            break;
        default:
            LOG(ERROR) << "received invalid operation parameters";
            return std::nullopt;
    }
    return operationParameters;
}

class HwCryptoOperationsNdk : public ndk_hwcrypto::BnHwCryptoOperations {
  private:
    sp<cpp_hwcrypto::IHwCryptoOperations> mHwCryptoOperations;

  public:
    HwCryptoOperationsNdk(sp<cpp_hwcrypto::IHwCryptoOperations> operations)
        : mHwCryptoOperations(std::move(operations)) {}

    static std::shared_ptr<HwCryptoOperationsNdk> Create(
            sp<cpp_hwcrypto::IHwCryptoOperations> operations) {
        if (operations == nullptr) {
            return nullptr;
        }
        std::shared_ptr<HwCryptoOperationsNdk> operationsNdk =
                ndk::SharedRefBase::make<HwCryptoOperationsNdk>(std::move(operations));

        if (!operationsNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoOperations";
            return nullptr;
        }
        return operationsNdk;
    }

    ndk::ScopedAStatus processCommandList(
            std::vector<ndk_hwcrypto::CryptoOperationSet>* operationSets,
            std::vector<ndk_hwcrypto::CryptoOperationResult>* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        if (operationSets == nullptr) {
            LOG(ERROR) << "received a null operation set";
            return convertStatus(status);
        }
        std::vector<cpp_hwcrypto::CryptoOperationResult> binderResult;
        std::vector<cpp_hwcrypto::CryptoOperationSet> cppOperationSets;
        for (ndk_hwcrypto::CryptoOperationSet& operationSet : *operationSets) {
            cpp_hwcrypto::CryptoOperationSet cppSingleOperation =
                    cpp_hwcrypto::CryptoOperationSet();
            cppSingleOperation.context = convertOperationsContext(operationSet.context);
            for (ndk_hwcrypto::CryptoOperation& operation : operationSet.operations) {
                cpp_hwcrypto::CryptoOperation cppOperation;
                cpp_hwcrypto::types::Void voidObj;
                std::optional<cpp_hwcrypto::types::OperationData> cppOperationData;
                std::optional<cpp_hwcrypto::PatternParameters> cppPatternParameters;
                std::optional<cpp_hwcrypto::OperationParameters> cppOperationParameters;
                switch (operation.getTag()) {
                    case ndk_hwcrypto::CryptoOperation::setMemoryBuffer:
                        // TODO: finish this case
                        exit(1);
                        break;
                    case ndk_hwcrypto::CryptoOperation::setOperationParameters:
                        cppOperationParameters = convertOperationParameters(
                                operation.get<
                                        ndk_hwcrypto::CryptoOperation::setOperationParameters>());
                        if (cppOperationParameters.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::setOperationParameters>(
                                    std::move(cppOperationParameters.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert operation parameters";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::setPattern:
                        cppPatternParameters = convertPatternParameters(
                                operation.get<ndk_hwcrypto::CryptoOperation::setPattern>());
                        if (cppPatternParameters.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::setPattern>(
                                    std::move(cppPatternParameters.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert pattern parameters";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::copyData:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::copyData>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::copyData>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::copyData";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::aadInput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::aadInput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::aadInput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::aadInput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::dataInput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::dataInput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::dataInput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::dataInput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::dataOutput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::dataOutput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::dataOutput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::dataOutput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::destroyContext:
                        cppOperation.set<cpp_hwcrypto::CryptoOperation::destroyContext>(
                                std::move(voidObj));
                        break;
                    case ndk_hwcrypto::CryptoOperation::finish:
                        cppOperation.set<cpp_hwcrypto::CryptoOperation::finish>(std::move(voidObj));
                        break;
                    default:
                        // This shouldn't happen
                        LOG(ERROR) << "received unknown crypto operation";
                        return convertStatus(status);
                }
                cppSingleOperation.operations.push_back(std::move(cppOperation));
            }
            cppOperationSets.push_back(std::move(cppSingleOperation));
        }
        status = mHwCryptoOperations->processCommandList(&cppOperationSets, &binderResult);
        if (status.isOk()) {
            *aidl_return = std::vector<ndk_hwcrypto::CryptoOperationResult>();
            for (cpp_hwcrypto::CryptoOperationResult& result : binderResult) {
                ndk_hwcrypto::CryptoOperationResult ndkResult =
                        ndk_hwcrypto::CryptoOperationResult();
                insertWrappedContextIntoMap(result.context, &ndkResult.context);
                aidl_return->push_back(std::move(ndkResult));
            }
        } else {
            // No reason to copy back the data output vectors if this failed
            LOG(ERROR) << "couldn't process command list";
            return convertStatus(status);
        }
        // We need to copy the vectors from the cpp operations back to the ndk one
        if (cppOperationSets.size() != operationSets->size()) {
            LOG(ERROR) << "ndk and cpp operation sets had a different number of elements";
            return convertStatus(Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
        }
        for (unsigned setIdx = 0; setIdx < cppOperationSets.size(); ++setIdx) {
            if (cppOperationSets[setIdx].operations.size() !=
                (*operationSets)[setIdx].operations.size()) {
                LOG(ERROR) << "ndk and cpp operations on set " << setIdx
                           << " had a different number of elements";
                return convertStatus(Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
            }
            for (unsigned operationIdx = 0;
                 operationIdx < cppOperationSets[setIdx].operations.size(); ++operationIdx) {
                if (cppOperationSets[setIdx].operations[operationIdx].getTag() ==
                    cpp_hwcrypto::CryptoOperation::dataOutput) {
                    if ((*operationSets)[setIdx].operations[operationIdx].getTag() !=
                        ndk_hwcrypto::CryptoOperation::dataOutput) {
                        LOG(ERROR)
                                << "ndk and cpp operations on set " << setIdx << " and operation "
                                << operationIdx << " had a different operation type";
                        return convertStatus(
                                Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
                    }
                    if (cppOperationSets[setIdx]
                                .operations[operationIdx]
                                .get<cpp_hwcrypto::CryptoOperation::dataOutput>()
                                .getTag() == cpp_hwcrypto::types::OperationData::dataBuffer) {
                        // This is the only case on which we need to move the data backto the
                        // original array
                        if ((*operationSets)[setIdx]
                                    .operations[operationIdx]
                                    .get<ndk_hwcrypto::CryptoOperation::dataOutput>()
                                    .getTag() != ndk_hwcrypto::types::OperationData::dataBuffer) {
                            LOG(ERROR) << "ndk and cpp operations on set " << setIdx
                                       << " and operation " << operationIdx
                                       << " had a different operation data output type";
                            return convertStatus(
                                    Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
                        }
                        (*operationSets)[setIdx]
                                .operations[operationIdx]
                                .get<ndk_hwcrypto::CryptoOperation::dataOutput>()
                                .set<ndk_hwcrypto::types::OperationData::dataBuffer>(
                                        cppOperationSets[setIdx]
                                                .operations[operationIdx]
                                                .get<cpp_hwcrypto::CryptoOperation::dataOutput>()
                                                .get<cpp_hwcrypto::types::OperationData::
                                                             dataBuffer>());
                    }
                }
            }
        }
        return convertStatus(status);
    }
};

class OpaqueKeyNdk : public ndk_hwcrypto::BnOpaqueKey {
  private:
    sp<cpp_hwcrypto::IOpaqueKey> mOpaqueKey;

  public:
    OpaqueKeyNdk(sp<cpp_hwcrypto::IOpaqueKey> opaqueKey) : mOpaqueKey(std::move(opaqueKey)) {}

    static std::shared_ptr<OpaqueKeyNdk> Create(sp<cpp_hwcrypto::IOpaqueKey> opaqueKey) {
        if (opaqueKey == nullptr) {
            return nullptr;
        }
        std::shared_ptr<OpaqueKeyNdk> opaqueKeyNdk =
                ndk::SharedRefBase::make<OpaqueKeyNdk>(std::move(opaqueKey));

        if (!opaqueKeyNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoKey";
            return nullptr;
        }
        return opaqueKeyNdk;
    }

    ndk::ScopedAStatus exportWrappedKey(
            const std::shared_ptr<ndk_hwcrypto::IOpaqueKey>& wrappingKey,
            ::std::vector<uint8_t>* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        auto wrappingKeyNdk = convertKeyPointer(wrappingKey);
        if (wrappingKeyNdk == nullptr) {
            LOG(ERROR) << "couldn't get wrapped key";
            return convertStatus(status);
        }
        status = mOpaqueKey->exportWrappedKey(wrappingKeyNdk, aidl_return);
        return convertStatus(status);
    }

    ndk::ScopedAStatus getKeyPolicy(ndk_hwcrypto::KeyPolicy* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        cpp_hwcrypto::KeyPolicy cppPolicy = cpp_hwcrypto::KeyPolicy();

        status = mOpaqueKey->getKeyPolicy(&cppPolicy);
        if (status.isOk()) {
            auto ndkPolicy = convertKeyPolicy(cppPolicy);
            if (ndkPolicy.has_value()) {
                *aidl_return = std::move(ndkPolicy.value());
            } else {
                LOG(ERROR) << "couldn't convert key policy";
                return ndk::ScopedAStatus::fromServiceSpecificError(
                        ndk_hwcrypto::types::HalErrorCode::BAD_PARAMETER);
            }
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus getPublicKey(::std::vector<uint8_t>* aidl_return) {
        auto status = mOpaqueKey->getPublicKey(aidl_return);
        return convertStatus(status);
    }

    ndk::ScopedAStatus getShareableToken(const ::std::vector<uint8_t>& sealingDicePolicy,
                                         ndk_hwcrypto::types::OpaqueKeyToken* aidl_return) {
        cpp_hwcrypto::types::OpaqueKeyToken binder_return;
        auto status = mOpaqueKey->getShareableToken(sealingDicePolicy, &binder_return);
        if (status.isOk()) {
            // TOO: check correctness
            aidl_return->keyToken = std::move(binder_return.keyToken);
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus setProtectionId(
            const ndk_hwcrypto::types::ProtectionId /*protectionId*/,
            const ::std::vector<ndk_hwcrypto::types::OperationType>& /*allowedOperations*/) {
        return ndk::ScopedAStatus::ok();
    }
};

void insertWrappedKeyIntoMap(sp<cpp_hwcrypto::IOpaqueKey> cppOpaqueKey,
                             std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* ndkOpaqueKey) {
    std::shared_ptr<ndk_hwcrypto::IOpaqueKey> opaqueKey = OpaqueKeyNdk::Create(cppOpaqueKey);
    std::weak_ptr<ndk_hwcrypto::IOpaqueKey> weakKeyNdk = opaqueKey;
    wp<cpp_hwcrypto::IOpaqueKey> weakKeyCpp = cppOpaqueKey;
    keyMapping.insert({weakKeyNdk, weakKeyCpp});
    *ndkOpaqueKey = opaqueKey;
}

Result<void> HwCryptoKey::connectToTrusty(const char* tipcDev) {
    assert(!mSession);
    mSession = RpcTrustyConnectWithSessionInitializer(tipcDev, HWCRYPTO_KEY_PORT, [](auto) {});
    if (!mSession) {
        return ErrnoError() << "failed to connect to hwcrypto";
    }
    mRoot = mSession->getRootObject();
    mHwCryptoServer = cpp_hwcrypto::IHwCryptoKey::asInterface(mRoot);
    return {};
}

HwCryptoKey::HwCryptoKey() {}

std::shared_ptr<HwCryptoKey> HwCryptoKey::Create(const char* tipcDev) {
    std::shared_ptr<HwCryptoKey> hwCrypto = ndk::SharedRefBase::make<HwCryptoKey>();

    if (!hwCrypto) {
        LOG(ERROR) << "failed to allocate HwCryptoKey";
        return nullptr;
    }

    auto ret = hwCrypto->connectToTrusty(tipcDev);
    if (!ret.ok()) {
        LOG(ERROR) << "failed to connect HwCryptoKey to Trusty: " << ret.error();
        return nullptr;
    }

    return hwCrypto;
}

ndk::ScopedAStatus HwCryptoKey::deriveCurrentDicePolicyBoundKey(
        const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
        ndk_hwcrypto::IHwCryptoKey::DiceCurrentBoundKeyResult* /*aidl_return*/) {
    // return mHwCryptoServer->deriveCurrentDicePolicyBoundKey(derivationKey, aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HwCryptoKey::deriveDicePolicyBoundKey(
        const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
        const ::std::vector<uint8_t>& /*dicePolicyForKeyVersion*/,
        ndk_hwcrypto::IHwCryptoKey::DiceBoundKeyResult* /*aidl_return*/) {
    // return mHwCryptoServer->deriveDicePolicyBoundKey(derivationKey, dicePolicyForKeyVersion,
    // aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HwCryptoKey::deriveKey(
        const ndk_hwcrypto::IHwCryptoKey::DerivedKeyParameters& /*parameters*/,
        ndk_hwcrypto::IHwCryptoKey::DerivedKey* /*aidl_return*/) {
    // return mHwCryptoServer->deriveKey(parameters, aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HwCryptoKey::getHwCryptoOperations(
        std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations>* aidl_return) {
    sp<cpp_hwcrypto::IHwCryptoOperations> binder_return;
    auto status = mHwCryptoServer->getHwCryptoOperations(&binder_return);
    if (status.isOk()) {
        std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations> operations =
                HwCryptoOperationsNdk::Create(binder_return);
        *aidl_return = operations;
    }
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::importClearKey(
        const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial,
        const ndk_hwcrypto::KeyPolicy& newKeyPolicy,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
    sp<cpp_hwcrypto::IOpaqueKey> binder_return;
    Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    auto cppKeyPolicy = convertKeyPolicy(newKeyPolicy);
    if (!cppKeyPolicy.has_value()) {
        LOG(ERROR) << "couldn't convert key policy";
        return convertStatus(status);
    }
    auto explicitKeyCpp = convertExplicitKeyMaterial(keyMaterial);
    if (!explicitKeyCpp.has_value()) {
        LOG(ERROR) << "couldn't convert key material";
        return convertStatus(status);
    }
    status = mHwCryptoServer->importClearKey(explicitKeyCpp.value(), cppKeyPolicy.value(),
                                             &binder_return);
    if (status.isOk()) {
        insertWrappedKeyIntoMap(binder_return, aidl_return);
    }
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::getCurrentDicePolicy(std::vector<uint8_t>* aidl_return) {
    auto status = mHwCryptoServer->getCurrentDicePolicy(aidl_return);
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::keyTokenImport(
        const ndk_hwcrypto::types::OpaqueKeyToken& requestedKey,
        const ::std::vector<uint8_t>& sealingDicePolicy,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
    sp<cpp_hwcrypto::IOpaqueKey> binder_return;
    cpp_hwcrypto::types::OpaqueKeyToken requestedKeyCpp;
    // trying first a shallow copy of the vector
    requestedKeyCpp.keyToken = requestedKey.keyToken;
    auto status =
            mHwCryptoServer->keyTokenImport(requestedKeyCpp, sealingDicePolicy, &binder_return);
    if (status.isOk()) {
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey> opaqueKey = OpaqueKeyNdk::Create(binder_return);
        *aidl_return = opaqueKey;
    }
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::getKeyslotData(
        ndk_hwcrypto::IHwCryptoKey::KeySlot /*slotId*/,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* /*aidl_return*/) {
    return ndk::ScopedAStatus::fromServiceSpecificError(
            ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED);
}

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android
