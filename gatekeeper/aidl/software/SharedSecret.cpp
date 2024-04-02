/*
 * Copyright 2024, The Android Open Source Project
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

#include "SharedSecret.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include <openssl/cmac.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <KeyMintUtils.h>
#include <aidl/android/hardware/security/sharedsecret/BnSharedSecret.h>
#include <aidl/android/hardware/security/sharedsecret/SharedSecretParameters.h>
#include <keymaster/android_keymaster_messages.h>
#include <keymaster/km_openssl/ckdf.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include "keymaster/android_keymaster_utils.h"

#include "SoftGateKeeper.h"

using keymaster::OpenSslObjectDeleter;
using keymaster::UniquePtr;

namespace aidl::android::hardware::security::sharedsecret {

::ndk::ScopedAStatus SoftSharedSecret::getSharedSecretParameters(
        SharedSecretParameters* out_params) {
    out_params->seed = std::vector<std::uint8_t>(32, 0);
    if (nonce_.empty()) {
        nonce_.resize(32, 0);
        RAND_bytes(nonce_.data(), 32);
    }
    out_params->nonce = nonce_;
    return ::ndk::ScopedAStatus::ok();
}

namespace {

DEFINE_OPENSSL_OBJECT_POINTER(HMAC_CTX);

keymaster_error_t hmacSha256(const keymaster_key_blob_t& key, const keymaster_blob_t data_chunks[],
                             size_t data_chunk_count, keymaster::KeymasterBlob* output) {
    if (!output) return KM_ERROR_UNEXPECTED_NULL_POINTER;

    unsigned digest_len = SHA256_DIGEST_LENGTH;
    if (!output->Reset(digest_len)) return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    HMAC_CTX_Ptr ctx(HMAC_CTX_new());
    if (!HMAC_Init_ex(ctx.get(), key.key_material, key.key_material_size, EVP_sha256(),
                      nullptr /* engine*/)) {
        return keymaster::TranslateLastOpenSslError();
    }

    for (size_t i = 0; i < data_chunk_count; i++) {
        auto& chunk = data_chunks[i];
        if (!HMAC_Update(ctx.get(), chunk.data, chunk.data_length)) {
            return keymaster::TranslateLastOpenSslError();
        }
    }

    if (!HMAC_Final(ctx.get(), output->writable_data(), &digest_len)) {
        return keymaster::TranslateLastOpenSslError();
    }

    if (digest_len != output->data_length) return KM_ERROR_UNKNOWN_ERROR;

    return KM_ERROR_OK;
}

}  // namespace

::ndk::ScopedAStatus SoftSharedSecret::computeSharedSecret(
        const std::vector<SharedSecretParameters>& params, std::vector<uint8_t>* sharing_check) {
    // Reimplemented based on SoftKeymasterEnforcement, which does not expose
    // enough functionality to satisfy the GateKeeper interface
    if (nonce_.empty() || hmac_key_.size() > 0) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    keymaster::KeymasterKeyBlob key_agreement_key;
    if (key_agreement_key.Reset(32) == nullptr) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::memset(key_agreement_key.writable_data(), 0, 32);
    constexpr const char kSharedHmacLabel[] = "KeymasterSharedMac";
    keymaster::KeymasterBlob label;
    if (label.Reset(sizeof(kSharedHmacLabel) - 1) == nullptr) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::memcpy(label.writable_data(), kSharedHmacLabel, sizeof(kSharedHmacLabel) - 1);

    static_assert(sizeof(keymaster_blob_t) == sizeof(keymaster::KeymasterBlob));

    bool found_mine = false;
    std::vector<keymaster::KeymasterBlob> context_blobs;
    for (const auto& param : params) {
        auto seed_blob = context_blobs.emplace_back();
        if (seed_blob.Reset(param.seed.size()) == nullptr) {
            return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        std::copy(param.seed.begin(), param.seed.end(), seed_blob.writable_data());
        auto nonce_blob = context_blobs.emplace_back();
        if (nonce_blob.Reset(param.seed.size()) == nullptr) {
            return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        std::copy(param.nonce.begin(), param.nonce.end(), nonce_blob.writable_data());
        auto pred = [](auto i) -> bool { return i == 0; };
        if (std::all_of(param.seed.begin(), param.seed.end(), pred)) {
            found_mine = true;
        }
    }
    if (!found_mine) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto context_blobs_ptr = reinterpret_cast<keymaster_blob_t*>(context_blobs.data());
    auto error = keymaster::ckdf(key_agreement_key, label, context_blobs_ptr, context_blobs.size(),
                                 &hmac_key_);
    if (error != KM_ERROR_OK) {
        return keymint::km_utils::kmError2ScopedAStatus(error);
    }

    constexpr const char kMacVerificationString[] = "Keymaster HMAC Verification";
    keymaster::KeymasterBlob verification_input;
    if (verification_input.Reset(sizeof(kMacVerificationString) - 1) == nullptr) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::memcpy(verification_input.writable_data(), kMacVerificationString,
                sizeof(kMacVerificationString) - 1);

    keymaster::KeymasterBlob verification_output;
    error = hmacSha256(hmac_key_, &verification_input, 1, &verification_output);
    if (error != KM_ERROR_OK) {
        return keymint::km_utils::kmError2ScopedAStatus(error);
    }
    *sharing_check = keymint::km_utils::kmBlob2vector(verification_output);

    return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::security::sharedsecret
