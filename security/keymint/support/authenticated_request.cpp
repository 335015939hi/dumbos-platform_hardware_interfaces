/*
 * Copyright 2024 The Android Open Source Project
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

#include <iomanip>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include "aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h"

#include <aidl/android/hardware/security/keymint/RpcHardwareInfo.h>
#include <android-base/macros.h>
#include <android-base/properties.h>
#include <cppbor.h>
#include <json/json.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/km_openssl/ec_key.h>
#include <keymaster/km_openssl/ecdsa_operation.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <openssl/base64.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include "include/remote_prov/remote_prov_utils_common.h"

#include <authenticated_request.h>

namespace aidl::android::hardware::security::keymint::remote_prov {

using cppcose::bytevec;
using cppcose::CoseKey;
using cppcose::CoseKeyCurve;
using cppcose::ErrMsgOr;

using keymaster::X509_Ptr;

using CRYPTO_BUFFER_Ptr = bssl::UniquePtr<CRYPTO_BUFFER>;

ErrMsgOr<hwtrust::DiceChain::Kind> getDiceChainKind() {
    int vendor_api_level = ::android::base::GetIntProperty("ro.vendor.api_level", -1);
    if (vendor_api_level == __ANDROID_API_T__) {
        return hwtrust::DiceChain::Kind::kVsr13;
    } else if (vendor_api_level == __ANDROID_API_U__) {
        return hwtrust::DiceChain::Kind::kVsr14;
    } else if (vendor_api_level == 202404) {
        return hwtrust::DiceChain::Kind::kVsr15;
    } else if (vendor_api_level > 202404) {
        return hwtrust::DiceChain::Kind::kVsr16;
    } else {
        return "Unsupported vendor API level: " + std::to_string(vendor_api_level);
    }
}

ErrMsgOr<X509_Ptr> parseX509Cert(const std::vector<uint8_t>& cert) {
    CRYPTO_BUFFER_Ptr certBuf(CRYPTO_BUFFER_new(cert.data(), cert.size(), nullptr));
    if (!certBuf.get()) {
        return "Failed to create crypto buffer.";
    }
    X509_Ptr result(X509_parse_from_buffer(certBuf.get()));
    if (!result.get()) {
        return "Failed to parse certificate.";
    }
    return result;
}

std::string getX509IssuerName(const X509_Ptr& cert) {
    char* name = X509_NAME_oneline(X509_get_issuer_name(cert.get()), nullptr, 0);
    std::string result(name);
    OPENSSL_free(name);
    return result;
}

std::string getX509SubjectName(const X509_Ptr& cert) {
    char* name = X509_NAME_oneline(X509_get_subject_name(cert.get()), nullptr, 0);
    std::string result(name);
    OPENSSL_free(name);
    return result;
}

// Validates the certificate chain and returns the leaf public key.
ErrMsgOr<bytevec> validateCertChain(const cppbor::Array& chain) {
    bytevec rawPubKey;
    for (size_t i = 0; i < chain.size(); ++i) {
        // Root must be self-signed.
        size_t signingCertIndex = (i > 0) ? i - 1 : i;
        auto& keyCertItem = chain[i];
        auto& signingCertItem = chain[signingCertIndex];
        if (!keyCertItem || !keyCertItem->asBstr()) {
            return "Key certificate must be a Bstr.";
        }
        if (!signingCertItem || !signingCertItem->asBstr()) {
            return "Signing certificate must be a Bstr.";
        }

        auto keyCert = parseX509Cert(keyCertItem->asBstr()->value());
        if (!keyCert) {
            return keyCert.message();
        }
        auto signingCert = parseX509Cert(signingCertItem->asBstr()->value());
        if (!signingCert) {
            return signingCert.message();
        }

        EVP_PKEY_Ptr pubKey(X509_get_pubkey(keyCert->get()));
        if (!pubKey.get()) {
            return "Failed to get public key.";
        }
        EVP_PKEY_Ptr signingPubKey(X509_get_pubkey(signingCert->get()));
        if (!signingPubKey.get()) {
            return "Failed to get signing public key.";
        }

        if (!X509_verify(keyCert->get(), signingPubKey.get())) {
            return "Verification of certificate " + std::to_string(i) +
                   " faile. OpenSSL error string: " + ERR_error_string(ERR_get_error(), NULL);
        }

        auto certIssuer = getX509IssuerName(*keyCert);
        auto signerSubj = getX509SubjectName(*signingCert);
        if (certIssuer != signerSubj) {
            return "Certificate " + std::to_string(i) + " has wrong issuer. Signer subject is " +
                   signerSubj + " Issuer subject is " + certIssuer;
        }
        if (i == chain.size() - 1) {
            auto key = getRawPublicKey(pubKey);
            if (!key) return key.moveMessage();
            rawPubKey = key.moveValue();
        }
    }
    return rawPubKey;
}

std::optional<std::string> validateUdsCerts(const cppbor::Map& udsCerts,
                                            const std::vector<uint8_t>& udsCoseKeyBytes) {
    for (const auto& [signerName, udsCertChain] : udsCerts) {
        if (!signerName || !signerName->asTstr()) {
            return "Signer Name must be a Tstr.";
        }
        if (!udsCertChain || !udsCertChain->asArray()) {
            return "UDS certificate chain must be an Array.";
        }
        if (udsCertChain->asArray()->size() < 2) {
            return "UDS certificate chain must have at least two entries: root and leaf.";
        }

        auto leafPubKey = validateCertChain(*udsCertChain->asArray());
        if (!leafPubKey) {
            return leafPubKey.message();
        }
        auto coseKey = CoseKey::parse(udsCoseKeyBytes);
        if (!coseKey) {
            return coseKey.moveMessage();
        }
        auto curve = coseKey->getIntValue(CoseKey::CURVE);
        if (!curve) {
            return "CoseKey must contain curve.";
        }
        bytevec udsPub;
        if (curve == CoseKeyCurve::P256 || curve == CoseKeyCurve::P384) {
            auto pubKey = coseKey->getEcPublicKey();
            if (!pubKey) {
                return pubKey.moveMessage();
            }
            // convert public key to uncompressed form by prepending 0x04 at begin.
            pubKey->insert(pubKey->begin(), 0x04);
            udsPub = pubKey.moveValue();
        } else if (curve == CoseKeyCurve::ED25519) {
            auto& pubkey = coseKey->getMap().get(cppcose::CoseKey::PUBKEY_X);
            if (!pubkey || !pubkey->asBstr()) {
                return "Invalid public key.";
            }
            udsPub = pubkey->asBstr()->value();
        } else {
            return "Unknown curve.";
        }
        if (*leafPubKey != udsPub) {
            return "Leaf public key in UDS certificate chain doesn't match UDS public key.";
        }
    }
    return std::nullopt;
}

std::optional<std::string> AuthenticatedRequest::parse() {
    auto [parsedRequest, _, csrErrMsg] = cppbor::parse(encodedRequest_);
    if (!parsedRequest) {
        return csrErrMsg;
    }
    if (!parsedRequest->asArray()) {
        return "AuthenticatedRequest is not a CBOR array.";
    }
    if (parsedRequest->asArray()->size() != 4U) {
        return "AuthenticatedRequest must contain version, UDS certificates, DICE chain, and "
               "signed data. However, the parsed AuthenticatedRequest has " +
               std::to_string(parsedRequest->asArray()->size()) + " entries.";
    }

    auto version = parsedRequest->asArray()->get(0)->asUint();
    auto udsCerts = parsedRequest->asArray()->get(1)->asMap();
    auto diceCertChain = parsedRequest->asArray()->get(2)->asArray();
    auto signedData = parsedRequest->asArray()->get(3)->asArray();

    if (!version) {
        return "AuthenticatedRequest version must be an unsigned integer.";
    }
    if (!udsCerts) {
        return "AuthenticatedRequest UdsCerts must be a Map.";
    }
    if (!diceCertChain) {
        return "AuthenticatedRequest DiceCertChain must be an Array.";
    }
    if (!signedData) {
        return "AuthenticatedRequest SignedData must be an Array.";
    }

    version_ = version->value();
    udsCerts_ = std::move(*udsCerts);
    diceCertChain_ = std::move(*diceCertChain);
    signedData_ = std::move(*signedData);

    return std::nullopt;
}

ErrMsgOr<bytevec> parseAndValidateSignedData(const std::vector<uint8_t>& payload,
                                             const std::vector<uint8_t>& challenge) {
    auto [signedData, _, errMsg] = cppbor::parse(payload);
    if (!signedData) {
        return errMsg;
    }
    if (!signedData->asArray()) {
        return "SignedData payload is not a CBOR array.";
    }
    if (signedData->asArray()->size() != 2U) {
        return "SignedData payload must contain the challenge and request. However, the parsed "
               "SignedData payload has " +
               std::to_string(signedData->asArray()->size()) + " entries.";
    }

    auto signedChallenge = signedData->asArray()->get(0)->asBstr();
    auto signedRequest = signedData->asArray()->get(1)->asBstr();

    if (!signedChallenge) {
        return "Challenge must be a Bstr.";
    }

    if (challenge.size() > 64) {
        return "Challenge size must be between 0 and 64 bytes inclusive. "
               "However, challenge is " +
               std::to_string(challenge.size()) + " bytes long.";
    }

    auto challengeBstr = cppbor::Bstr(challenge);
    if (*signedChallenge != challengeBstr) {
        return "Signed challenge does not match."
               "\n  Actual: " +
               cppbor::prettyPrint(signedChallenge->asBstr(), 64 /* maxBStrSize */) +
               "\nExpected: " + cppbor::prettyPrint(&challengeBstr, 64 /* maxBStrSize */);
    }

    if (!signedRequest) {
        return "Request must be a Bstr.";
    }

    return signedRequest->value();
}

ErrMsgOr<bytevec> AuthenticatedRequest::getUdsPubFromDiceChain_() {
    if (diceCertChain_.size() == 0 || !diceCertChain_.get(0)->asMap()) {
        return "AuthenticatedRequest DiceCertChain must be a non-empty Map";
    }

    auto udsPub = diceCertChain_.get(0)->asMap()->encode();
    return udsPub;
}

std::optional<std::string> AuthenticatedRequest::validate() {
    if (validated_) {
        return std::nullopt;
    }

    auto parsingError = parse();
    if (parsingError) {
        return *parsingError;
    }

    if (version_ != 1U) {
        return "AuthenticatedRequest version must be an unsigned integer and must be equal to 1.";
    }

    auto diceChainKind = getDiceChainKind();
    if (!diceChainKind) {
        return diceChainKind.message();
    }

    auto encodedDiceCertChain = diceCertChain_.encode();
    auto result = validateDiceCertChain(encodedDiceCertChain, *diceChainKind, allowAnyMode_,
                                        instanceName_);

    if (!result) {
        return result.message() + "\n" + prettyPrint(&diceCertChain_);
    }
    auto [isProper, diceCertChainEntries] = *result;

    isProper_ = isProper;

    if (!allowDegenerate_ && !isProper_) {
        return "AuthenticatedRequest DICE chain is degenerate.";
    }

    if (diceCertChainEntries.empty()) {
        return "AuthenticatedRequest no entries were in Dice Chain";
    }

    if (requireUdsCerts_ && udsCerts_.size() == 0) {
        return "AuthenticatedRequest UdsCerts must not be empty";
    }

    auto udsPub = getUdsPubFromDiceChain_();
    if (!udsPub) {
        return udsPub.message();
    }

    auto error = validateUdsCerts(udsCerts_, *udsPub);
    if (error) {
        return *error;
    }

    // could be UDS public key (if degenerate) or KeyMint public key (if proper)
    auto& leafSigningKey = diceCertChainEntries.back().pubKey;
    auto signedData = cppcose::verifyAndParseCoseSign1(&signedData_, leafSigningKey, /*aad=*/{});
    if (!signedData) {
        return signedData.message();
    }

    auto csrPayload = parseAndValidateSignedData(*signedData, challenge_);
    if (!csrPayload) {
        return csrPayload.message();
    }

    csrPayload_ = *csrPayload;

    validated_ = true;

    return std::nullopt;
}

ErrMsgOr<std::vector<uint8_t>> AuthenticatedRequest::csrPayload() {
    auto error = validate();
    if (error) {
        return *error;
    }
    return csrPayload_;
}

ErrMsgOr<bool> AuthenticatedRequest::isProper() {
    auto error = validate();
    if (error) {
        return *error;
    }
    return isProper_;
}

ErrMsgOr<bytevec> AuthenticatedRequest::getUdsPubFromDiceChain() {
    auto error = validate();
    if (error) {
        return *error;
    }

    return getUdsPubFromDiceChain_();
}
}  // namespace aidl::android::hardware::security::keymint::remote_prov