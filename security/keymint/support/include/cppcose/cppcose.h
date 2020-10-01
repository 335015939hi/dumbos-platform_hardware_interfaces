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

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <openssl/cipher.h>
#include <openssl/curve25519.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/hmac.h>
#include <openssl/mem.h>
#include <openssl/sha.h>

namespace cppcose {

using bytevec = std::vector<uint8_t>;

constexpr int kCoseSign1EntryCount = 4;
constexpr int kCoseSign1ProtectedParams = 0;
constexpr int kCoseSign1UnprotectedParams = 1;
constexpr int kCoseSign1Payload = 2;
constexpr int kCoseSign1Signature = 3;

constexpr int kCoseMac0EntryCount = 4;
constexpr int kCoseMac0ProtectedParams = 0;
constexpr int kCoseMac0UnprotectedParams = 1;
constexpr int kCoseMac0Payload = 2;
constexpr int kCoseMac0Tag = 3;

constexpr int kCoseEncryptEntryCount = 4;
constexpr int kCoseEncryptProtectedParams = 0;
constexpr int kCoseEncryptUnprotectedParams = 1;
constexpr int kCoseEncryptPayload = 2;
constexpr int kCoseEncryptRecipients = 3;

enum Label : int {
    ALGORITHM = 1,
    KEY_ID = 4,
    IV = 5,
    COSE_KEY = -1,
};

enum CoseKeyAlgorithm : int {
    AES_GCM_256 = 3,
    HMAC_256 = 5,
    EDDSA = -8,
    ECDH_ES_HKDF_256 = -25,
};

enum CoseKeyCurve : int { X25519 = 4, ED25519 = 6 };
enum CoseKeyType : int { OCTET_KEY_PAIR = 1, SYMMETRIC_KEY = 4 };

constexpr int kAesGcmNonceLength = 12;
constexpr int kAesGcmTagSize = 16;
constexpr int kAesGcmKeySize = 32;

template <typename T>
class ErrMsgOr {
  public:
    ErrMsgOr(std::string errMsg) : errMsg_(std::move(errMsg)) {}
    ErrMsgOr(const char* errMsg) : errMsg_(errMsg) {}
    ErrMsgOr(T val) : value_(std::move(val)) {}

    operator bool() const { return value_.has_value(); }

    T* operator->() & {
        assert(value_);
        return &value_.value();
    }
    T& operator*() & {
        assert(value_);
        return value_.value();
    };
    T&& operator*() && {
        assert(value_);
        return std::move(value_).value();
    };

    const std::string& message() { return errMsg_; }
    std::string moveMessage() { return std::move(errMsg_); }

    T moveValue() {
        assert(value_);
        return std::move(value_).value();
    }

  private:
    std::string errMsg_;
    std::optional<T> value_;
};

class CoseKey {
  public:
    CoseKey() {}
    CoseKey(const CoseKey&) = delete;
    CoseKey(CoseKey&&) = default;

    enum Label : int {
        KEY_TYPE = 1,
        KEY_ID = 2,
        ALGORITHM = 3,
        CURVE = -1,
        PUBKEY_X = -2,
        PRIVATE_KEY = -4,
        TEST_KEY = -70000  // Application-defined
    };

    static ErrMsgOr<CoseKey> parse(const bytevec& coseKey) {
        auto [parsedKey, _, errMsg] = cppbor::parse(coseKey);
        if (!parsedKey) return errMsg + " when parsing key";
        if (!parsedKey->asMap()) return "CoseKey must be a map";
        return CoseKey(static_cast<cppbor::Map*>(parsedKey.release()));
    }

    static ErrMsgOr<CoseKey> parse(const bytevec& coseKey, CoseKeyAlgorithm expectedAlgorithm,
                                   CoseKeyCurve expectedCurve) {
        auto key = parse(coseKey);
        if (!key) return key;

        if (!key->checkIntValue(CoseKey::KEY_TYPE, OCTET_KEY_PAIR) ||
            !key->checkIntValue(CoseKey::ALGORITHM, expectedAlgorithm) ||
            !key->checkIntValue(CoseKey::CURVE, expectedCurve)) {
            return "Unexpected key type";
        }

        return key;
    }

    static ErrMsgOr<CoseKey> parseEd25519(const bytevec& coseKey) {
        auto key = parse(coseKey, EDDSA, ED25519);
        if (!key) return key;

        auto& pubkey = key->getMap().get(PUBKEY_X);
        if (!pubkey || !pubkey->asBstr() ||
            pubkey->asBstr()->value().size() != ED25519_PUBLIC_KEY_LEN) {
            return "Invalid Ed25519 public key";
        }

        return key;
    }

    static ErrMsgOr<CoseKey> parseX25519(const bytevec& coseKey, bool requireKid) {
        auto key = parse(coseKey, ECDH_ES_HKDF_256, X25519);
        if (!key) return key;

        auto& pubkey = key->getMap().get(PUBKEY_X);
        if (!pubkey || !pubkey->asBstr() ||
            pubkey->asBstr()->value().size() != X25519_PUBLIC_VALUE_LEN) {
            return "Invalid X25519 public key";
        }

        auto& kid = key->getMap().get(KEY_ID);
        if (requireKid && (!kid || !kid->asBstr())) {
            return "Missing KID";
        }

        return key;
    }

    std::optional<int> getIntValue(Label label) {
        const auto& value = key_->get(label);
        if (!value || !value->asInt()) return {};
        return value->asInt()->value();
    }

    std::optional<bytevec> getBstrValue(Label label) {
        const auto& value = key_->get(label);
        if (!value || !value->asBstr()) return {};
        return value->asBstr()->value();
    }

    const cppbor::Map& getMap() const { return *key_; }
    cppbor::Map&& moveMap() { return std::move(*key_); }

    bool checkIntValue(Label label, int expectedValue) {
        const auto& value = key_->get(label);
        return value && value->asInt() && value->asInt()->value() == expectedValue;
    }

    void add(Label label, int value) { key_->add(label, value); }
    void add(Label label, bytevec value) { key_->add(label, std::move(value)); }

    bytevec encode() { return key_->canonicalize().encode(); }

  private:
    CoseKey(cppbor::Map* parsedKey) : key_(parsedKey) {}

    // This is the full parsed key structure.
    std::unique_ptr<cppbor::Map> key_;
};

ErrMsgOr<bytevec> generateCoseMac0Mac(const bytevec& macKey, const bytevec& externalAad,
                                      const bytevec& payload);
ErrMsgOr<cppbor::Array> constructCoseMac0(const bytevec& macKey, const bytevec& externalAad,
                                          const bytevec& payload);
ErrMsgOr<bytevec /* payload */> parseCoseMac0(const cppbor::Item* macItem);
ErrMsgOr<bytevec /* payload */> verifyAndParseCoseMac0(const cppbor::Item* macItem,
                                                       const bytevec& macKey);

ErrMsgOr<bytevec> createCoseSign1Signature(const bytevec& key, const bytevec& protectedParams,
                                           const bytevec& payload, const bytevec& aad);
ErrMsgOr<cppbor::Array> constructCoseSign1(const bytevec& key, const bytevec& payload,
                                           const bytevec& aad);
ErrMsgOr<cppbor::Array> constructCoseSign1(const bytevec& key, cppbor::Map extraProtectedFields,
                                           const bytevec& payload, const bytevec& aad);
/**
 * Verify and parse a COSE_Sign1 message, returning the payload.
 *
 * @param ignoreSignature indicates whether signature verification should be skipped.  If true, no
 *        verification of the signature will be done.
 *
 * @param coseSign1 is the COSE_Sign1 to verify and parse.
 *
 * @param signingCoseKey is a CBOR-encoded COSE_Key to use to verify the signature.  The bytevec may
 *        be empty, in which case the function assumes that coseSign1's payload is the COSE_Key to
 *        use, i.e. that coseSign1 is a self-signed "certificate".
 */
ErrMsgOr<bytevec /* payload */> verifyAndParseCoseSign1(bool ignoreSignature,
                                                        const cppbor::Array* coseSign1,
                                                        const bytevec& signingCoseKey,
                                                        const bytevec& aad);

ErrMsgOr<bytevec> createCoseEncryptCiphertext(const bytevec& key, const bytevec& nonce,
                                              const bytevec& protectedParams, const bytevec& aad);
ErrMsgOr<cppbor::Array> constructCoseEncrypt(const bytevec& key, const bytevec& nonce,
                                             const bytevec& plaintextPayload, const bytevec& aad,
                                             cppbor::Array recipients);
ErrMsgOr<std::pair<bytevec /* pubkey */, bytevec /* key ID */>> getSenderPubKeyFromCoseEncrypt(
        const cppbor::Item* encryptItem);
inline ErrMsgOr<std::pair<bytevec /* pubkey */, bytevec /* key ID */>>
getSenderPubKeyFromCoseEncrypt(const std::unique_ptr<cppbor::Item>& encryptItem) {
    return getSenderPubKeyFromCoseEncrypt(encryptItem.get());
}

ErrMsgOr<bytevec /* plaintextPayload */> decryptCoseEncrypt(const bytevec& key,
                                                            const cppbor::Item* encryptItem,
                                                            const bytevec& aad);

ErrMsgOr<bytevec> x25519_HKDF_DeriveKey(const bytevec& senderPubKey, const bytevec& senderPrivKey,
                                        const bytevec& recipientPubKey, bool senderIsA);

ErrMsgOr<bytevec /* ciphertextWithTag */> aesGcmEncrypt(const bytevec& key, const bytevec& nonce,
                                                        const bytevec& aad,
                                                        const bytevec& plaintext);
ErrMsgOr<bytevec /* plaintext */> aesGcmDecrypt(const bytevec& key, const bytevec& nonce,
                                                const bytevec& aad,
                                                const bytevec& ciphertextWithTag);

}  // namespace cppcose
