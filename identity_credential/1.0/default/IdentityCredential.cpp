/*
 * Copyright 2019, The Android Open Source Project
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

#include "IdentityCredential.h"

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredential follow.

Return<void> IdentityCredential::deleteCredential(deleteCredential_cb _hidl_cb) {
    return Void();
}

Return<void> IdentityCredential::createEphemeralKeyPair(KeyType keyType,
                                                        createEphemeralKeyPair_cb _hidl_cb) {
    return Void();
}

Return<void> IdentityCredential::startRetrieval(const StartRetrievalArguments& args,
                                                startRetrieval_cb _hidl_cb) {
    return Void();
}

Return<void> IdentityCredential::retrieveEntry(const SecureEntry& secureEntry,
                                               retrieveEntry_cb _hidl_cb) {
    return Void();
}

Return<void> IdentityCredential::finishRetrieval(const hidl_vec<uint8_t>& signingKeyBlob,
                                                 finishRetrieval_cb _hidl_cb) {
    return Void();
}

Return<void> IdentityCredential::generateSigningKeyPair(KeyType keyType,
                                                        generateSigningKeyPair_cb _hidl_cb) {
    return Void();
}

Return<ResultCode> IdentityCredential::provisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<hidl_vec<uint8_t>>& signingKeyData) {
    return ResultCode::FAILED;
}

Return<void> IdentityCredential::getDirectAccessSigningKeyPairStatus(
    getDirectAccessSigningKeyPairStatus_cb _hidl_cb) {
    return Void();
}

Return<ResultCode> IdentityCredential::deprovisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob) {
    return ResultCode::FAILED;
}

Return<ResultCode> IdentityCredential::configureDirectAccessPermissions(
    const hidl_vec<hidl_string>& itemsAllowedForDirectAccess) {
    return ResultCode::FAILED;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
