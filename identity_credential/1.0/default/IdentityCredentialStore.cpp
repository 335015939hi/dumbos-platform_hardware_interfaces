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

#define LOG_TAG "android.hardware.identity_credential@1.0-service"

#include "IdentityCredentialStore.h"
#include "IdentityCredential.h"
#include "WritableIdentityCredential.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredentialStore follow.

Return<void> IdentityCredentialStore::getHardwareInformation(getHardwareInformation_cb _hidl_cb) {
    _hidl_cb(ResultCode::OK, "test_store", "Google", 256u);
    LOG(INFO) << "Entering " << __FUNCTION__;
    return Void();
}

Return<void> IdentityCredentialStore::createCredential(const hidl_string& docType,
                                                       bool testCredential,
                                                       createCredential_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    auto writable_credential = new WritableIdentityCredential(docType, testCredential);
    _hidl_cb(ResultCode::OK, writable_credential);
    return Void();
}

Return<void> IdentityCredentialStore::getCredential(const hidl_vec<uint8_t>& credentialBlob,
                                                    getCredential_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    auto credential = new IdentityCredential(credentialBlob);
    if (!credential->initialize()) {
        _hidl_cb(ResultCode::FAILED, credential);
        return Void();
    }
    _hidl_cb(ResultCode::OK, credential);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
