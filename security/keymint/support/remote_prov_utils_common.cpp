/*
 * Copyright (c) 2024, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/hardware/security/keymint/RpcHardwareInfo.h>
#include <android-base/properties.h>
#include <hwtrust/hwtrust.h>
#include <keymaster/cppcose/cppcose.h>
#include <openssl/ec_key.h>

#include "include/remote_prov/remote_prov_utils_common.h"

namespace aidl::android::hardware::security::keymint::remote_prov {

using cppcose::BIGNUM_Ptr;
using cppcose::BN_CTX_Ptr;
using cppcose::EC_GROUP_Ptr;
using cppcose::EC_POINT_Ptr;

using keymaster::EC_KEY_Ptr;
using keymaster::X509_Ptr;

constexpr int kP256AffinePointSize = 32;

ErrMsgOr<bytevec> ecKeyGetPrivateKey(const EC_KEY* ecKey) {
    // Extract private key.
    const BIGNUM* bignum = EC_KEY_get0_private_key(ecKey);
    if (bignum == nullptr) {
        return "Error getting bignum from private key";
    }
    // Pad with zeros in case the length is lesser than 32.
    bytevec privKey(32, 0);
    BN_bn2binpad(bignum, privKey.data(), privKey.size());
    return privKey;
}

ErrMsgOr<bytevec> ecKeyGetPublicKey(const EC_KEY* ecKey, const int nid) {
    // Extract public key.
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(nid));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }
    const EC_POINT* point = EC_KEY_get0_public_key(ecKey);
    if (point == nullptr) return "Error getting ecpoint from public key";

    int size = EC_POINT_point2oct(group.get(), point, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0,
                                  nullptr);
    if (size == 0) {
        return "Error generating public key encoding";
    }

    bytevec publicKey;
    publicKey.resize(size);
    EC_POINT_point2oct(group.get(), point, POINT_CONVERSION_UNCOMPRESSED, publicKey.data(),
                       publicKey.size(), nullptr);
    return publicKey;
}

ErrMsgOr<bytevec> getRawPublicKey(const EVP_PKEY_Ptr& pubKey) {
    if (pubKey.get() == nullptr) {
        return "pkey is null.";
    }
    int keyType = EVP_PKEY_base_id(pubKey.get());
    switch (keyType) {
        case EVP_PKEY_EC: {
            int nid = EVP_PKEY_bits(pubKey.get()) == 384 ? NID_secp384r1 : NID_X9_62_prime256v1;
            auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pubKey.get()));
            if (ecKey.get() == nullptr) {
                return "Failed to get ec key";
            }
            return ecKeyGetPublicKey(ecKey.get(), nid);
        }
        case EVP_PKEY_ED25519: {
            bytevec rawPubKey;
            size_t rawKeySize = 0;
            if (!EVP_PKEY_get_raw_public_key(pubKey.get(), NULL, &rawKeySize)) {
                return "Failed to get raw public key.";
            }
            rawPubKey.resize(rawKeySize);
            if (!EVP_PKEY_get_raw_public_key(pubKey.get(), rawPubKey.data(), &rawKeySize)) {
                return "Failed to get raw public key.";
            }
            return rawPubKey;
        }
        default:
            return "Unknown key type.";
    }
}

ErrMsgOr<std::tuple<bytevec, bytevec>> getAffineCoordinates(const bytevec& pubKey) {
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), pubKey.data(), pubKey.size(), nullptr) != 1) {
        return "Error decoding publicKey";
    }
    BIGNUM_Ptr x(BN_new());
    BIGNUM_Ptr y(BN_new());
    BN_CTX_Ptr ctx(BN_CTX_new());
    if (!ctx.get()) return "Failed to create BN_CTX instance";

    if (!EC_POINT_get_affine_coordinates_GFp(group.get(), point.get(), x.get(), y.get(),
                                             ctx.get())) {
        return "Failed to get affine coordinates from ECPoint";
    }
    bytevec pubX(kP256AffinePointSize);
    bytevec pubY(kP256AffinePointSize);
    if (BN_bn2binpad(x.get(), pubX.data(), kP256AffinePointSize) != kP256AffinePointSize) {
        return "Error in converting absolute value of x coordinate to big-endian";
    }
    if (BN_bn2binpad(y.get(), pubY.data(), kP256AffinePointSize) != kP256AffinePointSize) {
        return "Error in converting absolute value of y coordinate to big-endian";
    }
    return std::make_tuple(std::move(pubX), std::move(pubY));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateEc256KeyPair() {
    auto ec_key = EC_KEY_Ptr(EC_KEY_new());
    if (ec_key.get() == nullptr) {
        return "Failed to allocate ec key";
    }

    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        return "Error generating key";
    }

    auto privKey = ecKeyGetPrivateKey(ec_key.get());
    if (!privKey) return privKey.moveMessage();

    auto pubKey = ecKeyGetPublicKey(ec_key.get(), NID_X9_62_prime256v1);
    if (!pubKey) return pubKey.moveMessage();

    return std::make_tuple(pubKey.moveValue(), privKey.moveValue());
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateX25519KeyPair() {
    /* Generate X25519 key pair */
    bytevec pubKey(X25519_PUBLIC_VALUE_LEN);
    bytevec privKey(X25519_PRIVATE_KEY_LEN);
    X25519_keypair(pubKey.data(), privKey.data());
    return std::make_tuple(std::move(pubKey), std::move(privKey));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateED25519KeyPair() {
    /* Generate ED25519 key pair */
    bytevec pubKey(ED25519_PUBLIC_KEY_LEN);
    bytevec privKey(ED25519_PRIVATE_KEY_LEN);
    ED25519_keypair(pubKey.data(), privKey.data());
    return std::make_tuple(std::move(pubKey), std::move(privKey));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateKeyPair(int32_t curve, bool forKeyExchange) {
    switch (curve) {
        case RpcHardwareInfo::CURVE_25519:
            if (forKeyExchange) {
                return generateX25519KeyPair();
            }
            return generateED25519KeyPair();
        case RpcHardwareInfo::CURVE_P256:
            return generateEc256KeyPair();
        default:
            return "Unsupported curve: " + std::to_string(curve);
    }
}

ErrMsgOr<std::tuple<bool, std::vector<DiceCertChainEntry>>> validateDiceCertChain(
        const bytevec& diceCertificateChain, hwtrust::DiceChain::Kind kind, bool allowAnyMode,
        const std::string& instanceName) {
    // Use ro.build.type instead of ro.debuggable because ro.debuggable=1 for VTS testing
    std::string build_type = ::android::base::GetProperty("ro.build.type", "");
    if (!build_type.empty() && build_type != "user") {
        allowAnyMode = true;
    }

    auto chain = hwtrust::DiceChain::Verify(diceCertificateChain, kind, allowAnyMode, instanceName);
    if (!chain.ok()) {
        return chain.error().message();
    }

    auto isProper = chain->IsProper();

    auto keys = chain->CosePublicKeys();
    if (!keys.ok()) {
        return keys.error().message();
    }
    std::vector<DiceCertChainEntry> result;
    for (auto& key : *keys) {
        result.push_back({std::move(key)});
    }
    return std::make_tuple(isProper, result);
}
}  // namespace aidl::android::hardware::security::keymint::remote_prov