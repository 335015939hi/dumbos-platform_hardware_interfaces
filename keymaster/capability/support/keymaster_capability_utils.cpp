/*
 * Copyright 2018 The Android Open Source Project
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

#include <keymaster_capability/keymaster_capability_utils.h>

#include <hardware/hw_auth_token.h>
#include <keymasterV4_0/keymaster_utils.h>

using namespace ::android::hardware;
using namespace ::android::hardware::keymaster::capability::V1_0;
using namespace ::android::hardware::keymaster::V4_0::support;

KeymasterCapability hidlVec2KeymasterCapability(hidl_vec<uint8_t> token) {
    KeymasterCapability capability;
    if (token.size() != sizeof(hw_auth_token_t)) return capability;

    capability.secure_token = std::move(token);

    auto v4_token = hidlVec2AuthToken(capability.secure_token);
    capability.challenge = v4_token.challenge;
    std::vector<uint64_t> ids;
    if (v4_token.userId) ids.push_back(v4_token.userId);
    if (v4_token.authenticatorId) ids.push_back(v4_token.authenticatorId);
    capability.ids = std::move(ids);
    capability.capabilityType = static_cast<CapabilityType>(v4_token.authenticatorType);
    capability.timestamp = v4_token.timestamp;

    return capability;
}
