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

#pragma once

#include <string>
#include <tuple>
#include <vector>

#include <hwtrust/hwtrust.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/km_openssl/openssl_utils.h>

using cppcose::bytevec;
using cppcose::ErrMsgOr;
using keymaster::EVP_PKEY_Ptr;

namespace aidl::android::hardware::security::keymint::remote_prov {

ErrMsgOr<bytevec> ecKeyGetPublicKey(const EC_KEY* ecKey, const int nid);

ErrMsgOr<bytevec> ecKeyGetPrivateKey(const EC_KEY* ecKey);

ErrMsgOr<bytevec> getRawPublicKey(const EVP_PKEY_Ptr& pubKey);

ErrMsgOr<std::tuple<bytevec, bytevec>> getAffineCoordinates(const bytevec& pubKey);

ErrMsgOr<std::tuple<bytevec, bytevec>> generateKeyPair(int32_t curve, bool forKeyExchange);

struct DiceCertChainEntry {
    bytevec pubKey;
};

ErrMsgOr<std::tuple<bool, std::vector<DiceCertChainEntry>>> validateDiceCertChain(
        const bytevec& diceCertChain, hwtrust::DiceChain::Kind kind, bool allowAnyMode,
        const std::string& instanceName);

}  // namespace aidl::android::hardware::security::keymint::remote_prov