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
#include <aidl/android/hardware/security/see/hwcrypto/BnHwCryptoKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnHwCryptoOperations.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnOpaqueKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/IHwCryptoKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/IOpaqueKey.h>
#include <android-base/logging.h>
#include <android-base/result.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/security/see/hwcrypto/BnHwCryptoKey.h>
#include <android/hardware/security/see/hwcrypto/IHwCryptoKey.h>
#include <binder/RpcServer.h>
#include <binder/RpcSession.h>
#include <binder/RpcTransportRaw.h>
#include <binder/RpcTransportTipcAndroid.h>
#include <binder/RpcTrusty.h>
#include <getopt.h>
#include <poll.h>
#include <trusty/tipc.h>
#include <memory>
#include <optional>
#include <string>

// We use cpp interfaces to talk to Trusty, and ndk interfaces for the platform
namespace cpp_hwcrypto = android::hardware::security::see::hwcrypto;
namespace ndk_hwcrypto = aidl::android::hardware::security::see::hwcrypto;

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

// TODO: Check refactoring opportunities like returning a Result<cpp_hwcrypto::types::OperationData>
//       once we add the code that uses this function.
Result<void> setOperationData(const ndk_hwcrypto::types::OperationData& ndkOperationData,
                              cpp_hwcrypto::types::OperationData* cppOperationData) {
    cpp_hwcrypto::types::MemoryBufferReference cppMemBuffRef;
    switch (ndkOperationData.getTag()) {
        case ndk_hwcrypto::types::OperationData::dataBuffer:
            cppOperationData->set<cpp_hwcrypto::types::OperationData::dataBuffer>(
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
            cppOperationData->set<cpp_hwcrypto::types::OperationData::memoryBufferReference>(
                    std::move(cppMemBuffRef));
            break;
        default:
            // This shouldn't happen with the current definitions
            return ErrnoError() << "received unknown operation data type";
    }
    return {};
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
            std::vector<ndk_hwcrypto::CryptoOperationSet>* /*operationSets*/,
            std::vector<ndk_hwcrypto::CryptoOperationResult>* /*aidl_return*/) {
        return ndk::ScopedAStatus::ok();
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

    ndk::ScopedAStatus getKeyPolicy(ndk_hwcrypto::KeyPolicy* /*aidl_return*/) {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus getPublicKey(::std::vector<uint8_t>* /*aidl_return*/) {
        return ndk::ScopedAStatus::ok();
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
        return ndk::ScopedAStatus::ok();
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

class HwCryptoKey : public ndk_hwcrypto::BnHwCryptoKey {
  private:
    sp<cpp_hwcrypto::IHwCryptoKey> mHwCryptoServer;
    sp<IBinder> mRoot;
    sp<RpcSession> mSession;

    Result<void> connectToTrusty(const char* tipcDev) {
        auto session_initializer = [](sp<RpcSession>& session) {
            session->setMaxIncomingThreads(1);
        };
        assert(!mSession);
        mSession = RpcTrustyConnectWithSessionInitializer(tipcDev, HWCRYPTO_KEY_PORT,
                                                          session_initializer);
        if (!mSession) {
            return ErrnoError() << "failed to connect to hwcrypto";
        }
        mRoot = mSession->getRootObject();
        mHwCryptoServer = cpp_hwcrypto::IHwCryptoKey::asInterface(mRoot);
        return {};
    }

  public:
    HwCryptoKey() {}

    static std::shared_ptr<HwCryptoKey> Create(const char* tipcDev) {
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

    ndk::ScopedAStatus deriveCurrentDicePolicyBoundKey(
            const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
            ndk_hwcrypto::IHwCryptoKey::DiceCurrentBoundKeyResult* /*aidl_return*/) {
        // return mHwCryptoServer->deriveCurrentDicePolicyBoundKey(derivationKey, aidl_return);
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus deriveDicePolicyBoundKey(
            const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
            const ::std::vector<uint8_t>& /*dicePolicyForKeyVersion*/,
            ndk_hwcrypto::IHwCryptoKey::DiceBoundKeyResult* /*aidl_return*/) {
        // return mHwCryptoServer->deriveDicePolicyBoundKey(derivationKey, dicePolicyForKeyVersion,
        // aidl_return);
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus deriveKey(
            const ndk_hwcrypto::IHwCryptoKey::DerivedKeyParameters& /*parameters*/,
            ndk_hwcrypto::IHwCryptoKey::DerivedKey* /*aidl_return*/) {
        // return mHwCryptoServer->deriveKey(parameters, aidl_return);
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus getHwCryptoOperations(
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

    ndk::ScopedAStatus importClearKey(const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial,
                                      const ndk_hwcrypto::KeyPolicy& newKeyPolicy,
                                      std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
        sp<cpp_hwcrypto::IOpaqueKey> binder_return;
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        auto cppKeyPolicy = convertKeyPolicy(newKeyPolicy);
        if (!cppKeyPolicy.has_value()) {
            return convertStatus(status);
        }
        auto explicitKeyCpp = convertExplicitKeyMaterial(keyMaterial);
        if (!explicitKeyCpp.has_value()) {
            return convertStatus(status);
        }
        status = mHwCryptoServer->importClearKey(explicitKeyCpp.value(), cppKeyPolicy.value(),
                                                 &binder_return);
        if (status.isOk()) {
            insertWrappedKeyIntoMap(binder_return, aidl_return);
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus getCurrentDicePolicy(std::vector<uint8_t>* aidl_return) {
        auto status = mHwCryptoServer->getCurrentDicePolicy(aidl_return);
        return convertStatus(status);
    }

    ndk::ScopedAStatus keyTokenImport(const ndk_hwcrypto::types::OpaqueKeyToken& requestedKey,
                                      const ::std::vector<uint8_t>& sealingDicePolicy,
                                      std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
        sp<cpp_hwcrypto::IOpaqueKey> binder_return;
        cpp_hwcrypto::types::OpaqueKeyToken requestedKeyCpp;
        // trying first a shallow copy of the vector
        requestedKeyCpp.keyToken = requestedKey.keyToken;
        auto status =
                mHwCryptoServer->keyTokenImport(requestedKeyCpp, sealingDicePolicy, &binder_return);
        if (status.isOk()) {
            std::shared_ptr<ndk_hwcrypto::IOpaqueKey> opaqueKey =
                    OpaqueKeyNdk::Create(binder_return);
            *aidl_return = opaqueKey;
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus getKeyslotData(ndk_hwcrypto::IHwCryptoKey::KeySlot slotId,
                                      std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        if (slotId == ndk_hwcrypto::IHwCryptoKey::KeySlot::KEYMINT_SHARED_HMAC_KEY) {
            sp<cpp_hwcrypto::IOpaqueKey> binder_return;
            status = mHwCryptoServer->getKeyslotData(
                    cpp_hwcrypto::IHwCryptoKey::KeySlot::KEYMINT_SHARED_HMAC_KEY, &binder_return);
            if (status.isOk()) {
                std::shared_ptr<ndk_hwcrypto::IOpaqueKey> opaqueKey =
                        OpaqueKeyNdk::Create(binder_return);
                *aidl_return = opaqueKey;
            }
            return convertStatus(status);
        } else {
            return convertStatus(status);
        }
    }
};

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android

static void showUsageAndExit(int code) {
    LOG(ERROR) << "usage: trusty-hwcryptohal-service -d <trusty_dev>";
    exit(code);
}

static void parseDeviceName(int argc, char* argv[], char*& device_name) {
    static const char* _sopts = "h:d:";
    static const struct option _lopts[] = {{"help", no_argument, nullptr, 'h'},
                                           {"trusty_dev", required_argument, nullptr, 'd'},
                                           {0, 0, 0, 0}};
    int opt;
    int oidx = 0;

    while ((opt = getopt_long(argc, argv, _sopts, _lopts, &oidx)) != -1) {
        switch (opt) {
            case 'd':
                device_name = strdup(optarg);
                break;
            case 'h':
                showUsageAndExit(EXIT_SUCCESS);
                break;
            default:
                LOG(ERROR) << "unrecognized option: " << opt;
                showUsageAndExit(EXIT_FAILURE);
        }
    }

    if (device_name == nullptr) {
        LOG(ERROR) << "missing required argument(s)";
        showUsageAndExit(EXIT_FAILURE);
    }

    LOG(INFO) << "starting trusty-hwcryptohal-service";
    LOG(INFO) << "trusty dev: " << device_name;
}

int main(int argc, char* argv[]) {
    char* device_name;
    parseDeviceName(argc, argv, device_name);

    auto hwCryptoServer = android::trusty::hwcryptohalservice::HwCryptoKey::Create(device_name);
    if (hwCryptoServer == nullptr) {
        LOG(ERROR) << "couldn't crete hwcrypto service";
        exit(EXIT_FAILURE);
    }
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    const std::string instance =
            std::string() + ndk_hwcrypto::IHwCryptoKey::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(hwCryptoServer->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "couldn't register hwcrypto service";
    }
    CHECK_EQ(status, STATUS_OK);
    ABinderProcess_joinThreadPool();

    return 0;
}
