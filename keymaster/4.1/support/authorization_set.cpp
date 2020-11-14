/*
 * Copyright 2020 The Android Open Source Project
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

#include <keymasterV4_1/authorization_set.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_1 {

AuthorizationSetBuilder& AuthorizationSet4_1Builder::OaepMGFDigest(
        std::vector<V4_0::Digest> digests) {
    for (auto digest : digests) {
        push_back(TAG_RSA_OAEP_MGF_DIGEST, digest);
    }
    return *this;
}

}  // namespace V4_1
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
