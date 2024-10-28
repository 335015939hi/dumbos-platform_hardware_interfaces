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

#include <cppbor.h>
#include <keymaster/cppcose/cppcose.h>

#include <string>

using cppbor::Array;
using cppbor::Map;
using cppcose::bytevec;
using cppcose::ErrMsgOr;

namespace aidl::android::hardware::security::keymint::remote_prov {

class AuthenticatedRequest {
  public:
    AuthenticatedRequest(const bytevec request, const bytevec challenge,
                         const std::string instanceName, bool allowAnyMode, bool allowDegenerate,
                         bool requireUdsCerts)
        : mEncodedRequest(request),
          mChallenge(challenge),
          mInstanceName(instanceName),
          mAllowAnyMode(allowAnyMode),
          mAllowDegenerate(allowDegenerate),
          mRequireUdsCerts(requireUdsCerts) {}

    ErrMsgOr<bytevec> csrPayload();

    ErrMsgOr<bool> isProper();

    ErrMsgOr<bytevec> getUdsPubFromDiceChain();

  private:
    bool mValidated = false;

    std::optional<std::string> parse();
    std::optional<std::string> validate();
    ErrMsgOr<bytevec> getUdsPubFromDiceChain_();

    const bytevec mEncodedRequest;
    const bytevec mChallenge;
    const std::string mInstanceName;
    const bool mAllowAnyMode;
    const bool mAllowDegenerate;
    const bool mRequireUdsCerts;

    uint mVersion;
    Map mUdsCerts;
    Array mDiceCertChain;
    bool mIsProper = false;
    Array mSignedData;

    bytevec mCsrPayload;
};
}  // namespace aidl::android::hardware::security::keymint::remote_prov