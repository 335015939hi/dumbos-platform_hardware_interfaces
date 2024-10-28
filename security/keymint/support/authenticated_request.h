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
#include <hwtrust/hwtrust.h>
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

class AuthenticatedRequest {
  public:
    AuthenticatedRequest(const bytevec request, const bytevec challenge,
                         const std::string instanceName, bool allowAnyMode, bool allowDegenerate,
                         bool requireUdsCerts)
        : encodedRequest_(request),
          challenge_(challenge),
          instanceName_(instanceName),
          allowAnyMode_(allowAnyMode),
          allowDegenerate_(allowDegenerate),
          requireUdsCerts_(requireUdsCerts) {}

    cppcose::ErrMsgOr<bytevec> csrPayload();

    cppcose::ErrMsgOr<bool> isProper();

    cppcose::ErrMsgOr<bytevec> getUdsPubFromDiceChain();

  private:
    bool validated_ = false;

    std::optional<std::string> parse();
    std::optional<std::string> validate();
    cppcose::ErrMsgOr<bytevec> getUdsPubFromDiceChain_();

    const bytevec encodedRequest_;
    const bytevec challenge_;
    const std::string instanceName_;
    bool allowAnyMode_;
    bool allowDegenerate_;
    bool requireUdsCerts_;

    uint version_;
    cppbor::Map udsCerts_;
    cppbor::Array diceCertChain_;
    bool isProper_ = false;
    cppbor::Array signedData_;

    bytevec csrPayload_;
};
}  // namespace aidl::android::hardware::security::keymint::remote_prov