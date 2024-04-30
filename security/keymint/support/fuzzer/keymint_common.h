/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __KEYMINT_COMMON_H__
#define __KEYMINT_COMMON_H__

#include <aidl/android/hardware/security/keymint/BlockMode.h>
#include <aidl/android/hardware/security/keymint/Digest.h>
#include <aidl/android/hardware/security/keymint/EcCurve.h>
#include <aidl/android/hardware/security/keymint/PaddingMode.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <keymint_support/authorization_set.h>

namespace android::hardware::security::keymint_support::fuzzer {

using namespace aidl::android::hardware::security::keymint;

constexpr uint32_t kKeySize = 2048;
constexpr uint32_t kStringSize = 256;
constexpr uint32_t kPublicExponent = 65537;

constexpr EcCurve kCurve[] = {EcCurve::P_224, EcCurve::P_256, EcCurve::P_384, EcCurve::P_521,
                              EcCurve::CURVE_25519};

constexpr PaddingMode kPaddingMode[] = {
        PaddingMode::NONE,
        PaddingMode::RSA_OAEP,
        PaddingMode::RSA_PSS,
        PaddingMode::RSA_PKCS1_1_5_ENCRYPT,
        PaddingMode::RSA_PKCS1_1_5_SIGN,
        PaddingMode::PKCS7,
};

constexpr Digest kDigest[] = {
        Digest::NONE,      Digest::MD5,       Digest::SHA1,      Digest::SHA_2_224,
        Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512,
};

constexpr BlockMode kBlockMode[] = {
        BlockMode::ECB,
        BlockMode::CBC,
        BlockMode::CTR,
        BlockMode::GCM,
};

enum AuthSet : uint32_t {
    RSA_KEY = 0u,
    RSA_SIGNING_KEY,
    RSA_ENCRYPRION_KEY,
    ECDSA_SIGNING_CURVE,
    ECDSA_SIGNING_KEY,
    AES_ENCRYPTION_KEY,
    TRIPLE_DES,
    HMAC,
    NO_DIGEST,
    ECB_MODE,
    GSM_MODE_MIN_MAC,
    GSM_MODE_MAC,
    BLOCK_MODE,
    kMaxValue = BLOCK_MODE
};

AuthorizationSet createAuthorizationSet(FuzzedDataProvider* dataProvider) {
    uint32_t authSet = dataProvider->ConsumeEnum<AuthSet>();
    Digest digest = dataProvider->PickValueInArray(kDigest);
    PaddingMode padding = dataProvider->PickValueInArray(kPaddingMode);
    switch (authSet) {
        case RSA_KEY: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaKey(kKeySize, kPublicExponent)
                    .Digest(digest)
                    .Padding(padding)
                    .AttestKey()
                    .SetDefaultValidity()
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case RSA_SIGNING_KEY: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaSigningKey(kKeySize, kPublicExponent)
                    .Digest(digest)
                    .Padding(padding)
                    .AttestationChallenge(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .AttestationApplicationId(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .SetDefaultValidity()
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case RSA_ENCRYPRION_KEY: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .RsaEncryptionKey(kKeySize, kPublicExponent)
                    .Digest(digest)
                    .Padding(padding)
                    .AttestationChallenge(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .AttestationApplicationId(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .SetDefaultValidity()
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECDSA_SIGNING_CURVE: {
            EcCurve ecCurve = dataProvider->PickValueInArray(kCurve);
            uint64_t timestamp = dataProvider->ConsumeIntegral<uint64_t>();
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey(ecCurve)
                    .Digest(digest)
                    .AttestationChallenge(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .AttestationApplicationId(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .SetDefaultValidity()
                    .Authorization(TAG_CREATION_DATETIME, timestamp)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECDSA_SIGNING_KEY: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcdsaSigningKey((EcCurve)kKeySize)
                    .Digest(digest)
                    .AttestationChallenge(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .AttestationApplicationId(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .SetDefaultValidity()
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case AES_ENCRYPTION_KEY: {
            BlockMode blockmode = dataProvider->PickValueInArray(kBlockMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .AesEncryptionKey(kKeySize)
                    .BlockMode(blockmode)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case TRIPLE_DES: {
            BlockMode blockmode = dataProvider->PickValueInArray(kBlockMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .TripleDesEncryptionKey(kKeySize)
                    .BlockMode(blockmode)
                    .Digest(digest)
                    .Padding(padding)
                    .EcbMode()
                    .AttestationChallenge(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .AttestationApplicationId(dataProvider->ConsumeRandomLengthString(kStringSize))
                    .SetDefaultValidity()
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case HMAC: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .HmacKey(kKeySize)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case NO_DIGEST: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .NoDigestOrPadding()
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case ECB_MODE: {
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .EcbMode()
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case GSM_MODE_MIN_MAC: {
            uint32_t minMacLength = dataProvider->ConsumeIntegral<uint32_t>();
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .GcmModeMinMacLen(minMacLength)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case GSM_MODE_MAC: {
            uint32_t macLength = dataProvider->ConsumeIntegral<uint32_t>();
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .GcmModeMacLen(macLength)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        case BLOCK_MODE: {
            BlockMode blockmode = dataProvider->PickValueInArray(kBlockMode);
            return AuthorizationSetBuilder()
                    .Authorization(TAG_NO_AUTH_REQUIRED)
                    .BlockMode(blockmode)
                    .Digest(digest)
                    .Padding(padding)
                    .Authorization(TAG_INCLUDE_UNIQUE_ID);
        } break;
        default:
            break;
    };
    return AuthorizationSetBuilder();
}

}  // namespace android::hardware::security::keymint_support::fuzzer

#endif  // __KEYMINT_COMMON_H__
