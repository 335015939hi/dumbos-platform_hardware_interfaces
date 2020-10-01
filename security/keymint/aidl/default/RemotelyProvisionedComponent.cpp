/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "RemotelyProvisionedComponent.h"

#include <assert.h>
#include <variant>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <cppcose/cppcose.h>
#include <remote_prov/remote_prov_utils.h>

#include <openssl/rand.h>

namespace aidl::android::hardware::security::keymint {

using ::std::string;
using ::std::tuple;
using ::std::unique_ptr;
using ::std::variant;
using ::std::vector;
using bytevec = ::std::vector<uint8_t>;

using namespace cppcose;

namespace {

constexpr auto STATUS_FAILED = RemotelyProvisionedComponent::STATUS_FAILED;
constexpr auto STATUS_INVALID_EEK = RemotelyProvisionedComponent::STATUS_INVALID_EEK;
constexpr auto STATUS_INVALID_MAC = RemotelyProvisionedComponent::STATUS_INVALID_MAC;

struct AStatusDeleter {
    void operator()(AStatus* p) { AStatus_delete(p); }
};

// TODO(swillden): Remove the dependency on AStatus stuff.  The COSE lib should use something like
// StatusOr, but it shouldn't depend on AStatus.
class Status {
  public:
    Status() {}
    Status(int32_t errCode, const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(errCode, errMsg.c_str())) {}
    explicit Status(const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(STATUS_FAILED, errMsg.c_str())) {}
    Status(AStatus* status) : status_(status) {}
    Status(Status&&) = default;
    Status(const Status&) = delete;

    operator ::ndk::ScopedAStatus() && { return ndk::ScopedAStatus(status_.release()); }

    bool isOk() { return !status_; }

    // Don't call getMessage() unless isOk() returns false;
    const char* getMessage() const { return AStatus_getMessage(status_.get()); }

  private:
    std::unique_ptr<AStatus, AStatusDeleter> status_;
};

template <typename T>
class StatusOr {
  public:
    StatusOr(AStatus* status) : status_(status) {}
    StatusOr(Status status) : status_(std::move(status)) {}
    StatusOr(T val) : value_(std::move(val)) {}

    bool isOk() { return status_.isOk(); }

    T* operator->() & {
        assert(isOk());
        return &value_.value();
    }
    T& operator*() & {
        assert(isOk());
        return value_.value();
    }
    T&& operator*() && {
        assert(isOk());
        return std::move(value_).value();
    }

    const char* getMessage() const {
        assert(!isOk());
        return status_.getMessage();
    }

    Status moveError() {
        assert(!isOk());
        return std::move(status_);
    }

    T moveValue() { return std::move(value_).value(); }

  private:
    Status status_;
    std::optional<T> value_;
};

StatusOr<std::pair<bytevec /* EEK pub */, bytevec /* EEK ID */>> validateAndExtractEekPubAndId(
        bool testMode, const bytevec& endpointEncryptionCertChain) {
    auto [item, newPos, errMsg] = cppbor::parse(endpointEncryptionCertChain);

    if (!item || !item->asArray()) {
        return Status("Error parsing EEK chain" + errMsg);
    }

    const cppbor::Array* certArr = item->asArray();
    bytevec lastPubKey;
    for (int i = 0; i < certArr->size(); ++i) {
        auto cosePubKey = verifyAndParseCoseSign1(testMode, certArr->get(i)->asArray(),
                                                  std::move(lastPubKey), bytevec{} /* AAD */);
        if (!cosePubKey) {
            return Status(STATUS_INVALID_EEK,
                          "Failed to validate EEK chain: " + cosePubKey.moveMessage());
        }
        lastPubKey = *std::move(cosePubKey);
    }

    auto eek = CoseKey::parseX25519(lastPubKey, true /* requireKid */);
    if (!eek) return Status(STATUS_INVALID_EEK, "Failed to get EEK: " + eek.moveMessage());

    return std::make_pair(eek->getBstrValue(CoseKey::PUBKEY_X).value(),
                          eek->getBstrValue(CoseKey::KEY_ID).value());
}

StatusOr<bytevec /* pubkeys */> validateAndExtractPubkeys(bool testMode,
                                                          const vector<MacedPublicKey>& keysToSign,
                                                          const bytevec& macKey) {
    auto pubKeysToMac = cppbor::Array();
    for (auto& keyToSign : keysToSign) {
        auto [macedKeyItem, _, coseMacErrMsg] = cppbor::parse(keyToSign.macedKey);
        if (!macedKeyItem || !macedKeyItem->asArray() ||
            macedKeyItem->asArray()->size() != kCoseMac0EntryCount) {
            return Status("Invalid COSE_Mac0 structure");
        }

        auto protectedParms = macedKeyItem->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
        auto unprotectedParms = macedKeyItem->asArray()->get(kCoseMac0UnprotectedParams)->asBstr();
        auto payload = macedKeyItem->asArray()->get(kCoseMac0Payload)->asBstr();
        auto tag = macedKeyItem->asArray()->get(kCoseMac0Tag)->asBstr();
        if (!protectedParms || !unprotectedParms || !payload || !tag) {
            return Status("Invalid COSE_Mac0 contents");
        }

        auto [protectedMap, __, errMsg] = cppbor::parse(protectedParms);
        if (!protectedMap || !protectedMap->asMap()) {
            return Status("Invalid Mac0 protected: " + errMsg);
        }
        auto& algo = protectedMap->asMap()->get(ALGORITHM);
        if (!algo || !algo->asInt() || algo->asInt()->value() != HMAC_256) {
            return Status("Unsupported Mac0 algorithm");
        }

        auto pubKey = CoseKey::parse(payload->value(), EDDSA, ED25519);
        if (!pubKey) return Status(pubKey.moveMessage());

        bool testKey = static_cast<bool>(pubKey->getMap().get(CoseKey::TEST_KEY));
        if (testMode && !testKey) {
            return Status(BnRemotelyProvisionedComponent::STATUS_PRODUCTION_KEY_IN_TEST_REQUEST,
                          "Production key in test request");
        } else if (!testMode && testKey) {
            return Status(BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST,
                          "Test key in production request");
        }

        auto macTag = generateCoseMac0Mac(macKey, {} /* external_aad */, payload->value());
        if (!macTag) return Status(STATUS_INVALID_MAC, macTag.moveMessage());
        if (macTag->size() != tag->value().size() ||
            CRYPTO_memcmp(macTag->data(), tag->value().data(), macTag->size()) != 0) {
            return Status(STATUS_INVALID_MAC, "MAC tag mismatch");
        }

        pubKeysToMac.add(pubKey->moveMap());
    }

    return pubKeysToMac.encode();
}

cppbor::Array buildCertReqRecipients(const bytevec& pubkey, const bytevec& kid) {
    return cppbor::Array()                   // Array of recipients
            .add(cppbor::Array()             // Recipient
                         .add(cppbor::Map()  // Protected
                                      .add(ALGORITHM, ECDH_ES_HKDF_256)
                                      .canonicalize()
                                      .encode())
                         .add(cppbor::Map()  // Unprotected
                                      .add(COSE_KEY, cppbor::Map()
                                                             .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                                                             .add(CoseKey::CURVE, cppcose::X25519)
                                                             .add(CoseKey::PUBKEY_X, pubkey)
                                                             .canonicalize())
                                      .add(KEY_ID, kid)
                                      .canonicalize())
                         .add(cppbor::Null()));  // No ciphertext
}

}  // namespace

ScopedAStatus RemotelyProvisionedComponent::generateEd25519KeyPair(bool testMode,
                                                                   MacedPublicKey* macedPublicKey,
                                                                   bytevec* privateKeyHandle) {
    privateKeyHandle->reserve(ED25519_PRIVATE_KEY_LEN);
    bytevec publicValue(ED25519_PUBLIC_KEY_LEN);

    // TODO(swillden): Probably shouldn't just return the private key :-)
    ED25519_keypair(publicValue.data(), privateKeyHandle->data());

    cppbor::Map cosePublicKeyMap = cppbor::Map()
                                           .add(1 /* Key type */, 1 /* Octet key pair */)
                                           .add(3 /* Algorithm */, -8 /* EdDSA */)
                                           .add(-1 /* Curve */, 6 /* Ed25519 */)
                                           .add(-2 /* Key value */, publicValue);

    if (testMode) {
        cosePublicKeyMap.add(-70000, cppbor::Null());
    }

    bytevec cosePublicKey = cosePublicKeyMap.canonicalize().encode();

    auto macedKey = constructCoseMac0(testMode ? remote_prov::kTestMacKey : macKey_,
                                      {} /* externalAad */, cosePublicKey);
    if (!macedKey) return Status(macedKey.moveMessage());

    macedPublicKey->macedKey = macedKey->encode();
    return ScopedAStatus::ok();
}

ScopedAStatus RemotelyProvisionedComponent::generateCertificateRequest(
        bool testMode, const vector<MacedPublicKey>& keysToSign,
        const bytevec& endpointEncCertChain, const bytevec& challenge, bytevec* keysToSignMac,
        bytevec* protectedData) {
    auto pubKeysToSign = validateAndExtractPubkeys(testMode, keysToSign,
                                                   testMode ? remote_prov::kTestMacKey : macKey_);
    if (!pubKeysToSign.isOk()) return pubKeysToSign.moveError();

    bytevec ephemeralMacKey = remote_prov::randomBytes(SHA256_DIGEST_LENGTH);

    auto pubKeysToSignMac = generateCoseMac0Mac(ephemeralMacKey, bytevec{}, *pubKeysToSign);
    if (!pubKeysToSignMac) return Status(pubKeysToSignMac.moveMessage());
    *keysToSignMac = *std::move(pubKeysToSignMac);

    bytevec devicePrivKey;
    cppbor::Array bcc;
    if (testMode) {
        std::tie(devicePrivKey, bcc) = generateBcc();
    } else {
        devicePrivKey = devicePrivKey_;
        bcc = bcc_.clone();
    }

    auto signedMac = constructCoseSign1(devicePrivKey /* Signing key */,  //
                                        ephemeralMacKey /* Payload */,
                                        cppbor::Array() /* AAD */
                                                .add(challenge)
                                                .add(createDeviceInfo())
                                                .encode());
    if (!signedMac) return Status(signedMac.moveMessage());

    bytevec ephemeralPrivKey(X25519_PRIVATE_KEY_LEN);
    bytevec ephemeralPubKey(X25519_PUBLIC_VALUE_LEN);
    X25519_keypair(ephemeralPubKey.data(), ephemeralPrivKey.data());

    auto eek = validateAndExtractEekPubAndId(testMode, endpointEncCertChain);
    if (!eek.isOk()) return eek.moveError();

    auto sessionKey = x25519_HKDF_DeriveKey(ephemeralPubKey, ephemeralPrivKey, eek->first,
                                            true /* senderIsA */);
    if (!sessionKey) return Status(sessionKey.moveMessage());

    auto coseEncrypted =
            constructCoseEncrypt(*sessionKey, remote_prov::randomBytes(kAesGcmNonceLength),
                                 cppbor::Array()  // payload
                                         .add(signedMac.moveValue())
                                         .add(std::move(bcc))
                                         .encode(),
                                 {},  // aad
                                 buildCertReqRecipients(ephemeralPubKey, eek->second));

    if (!coseEncrypted) return Status(coseEncrypted.moveMessage());
    *protectedData = coseEncrypted->encode();

    return ScopedAStatus::ok();
}

bytevec RemotelyProvisionedComponent::deriveBytesFromHbk(const string& context,
                                                         size_t numBytes) const {
    bytevec fakeHbk(32, 0);
    bytevec result(numBytes);

    // TODO(swillden): Figure out if HKDF can fail.  It doesn't seem like it should be able to,
    // but the function does return an error code.
    HKDF(result.data(), numBytes,               //
         EVP_sha256(),                          //
         fakeHbk.data(), fakeHbk.size(),        //
         nullptr /* salt */, 0 /* salt len */,  //
         reinterpret_cast<const uint8_t*>(context.data()), context.size());

    return result;
}

bytevec RemotelyProvisionedComponent::createDeviceInfo() const {
    return cppbor::Array().encode();
}

std::pair<bytevec /* privKey */, cppbor::Array /* BCC */>
RemotelyProvisionedComponent::generateBcc() {
    bytevec privKey(ED25519_PRIVATE_KEY_LEN);
    bytevec pubKey(ED25519_PUBLIC_KEY_LEN);

    ED25519_keypair(pubKey.data(), privKey.data());

    auto coseSign1 = constructCoseSign1(privKey,      /* signing key */
                                        cppbor::Map() /* extra protected */
                                                .add(-70001, remote_prov::randomBytes(32)),
                                        cppbor::Map() /* payload CoseKey */
                                                .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                                                .add(CoseKey::ALGORITHM, EDDSA)
                                                .add(CoseKey::CURVE, ED25519)
                                                .add(CoseKey::PUBKEY_X, pubKey)
                                                .canonicalize()
                                                .encode(),
                                        {} /* AAD */);
    assert(coseSign1);

    return {privKey, cppbor::Array().add(coseSign1.moveValue())};
}

}  // namespace aidl::android::hardware::security::keymint
