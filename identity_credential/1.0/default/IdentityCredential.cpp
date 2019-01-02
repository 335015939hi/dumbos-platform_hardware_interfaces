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

#include "IdentityCredential.h"

#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using std::vector;

using android::hardware::identity_credential::support::decryptAes128Gcm;
using android::hardware::identity_credential::support::encryptAes128Gcm;
using android::hardware::identity_credential::support::getRandom;
using android::hardware::identity_credential::support::getTestHardwareBoundKey;
using android::hardware::identity_credential::support::hexdump;

using android::hardware::identity_credential::support::cborArrayGetBStr;
using android::hardware::identity_credential::support::CnCborPtr;

using android::hardware::identity_credential::support::entryCreateAdditionalData;

// Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredential follow.

Return<void> IdentityCredential::deleteCredential(deleteCredential_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    _hidl_cb(ResultCode::FAILED, {});
    return Void();
}

Return<void> IdentityCredential::createEphemeralKeyPair(KeyType keyType,
                                                        createEphemeralKeyPair_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    _hidl_cb({});
    return Void();
}

Return<void> IdentityCredential::startRetrieval(const StartRetrievalArguments& args,
                                                startRetrieval_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;

    vector<uint8_t> decryptedCredentialKeys;
    if (!decryptAes128Gcm(getTestHardwareBoundKey(), credentialBlob_, {},
                          decryptedCredentialKeys)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    // hexdump("decrypted credentialBlob", decryptedCredentialKeys);

    cn_cbor_errback err;
    auto cbor = CnCborPtr(
        cn_cbor_decode(decryptedCredentialKeys.data(), decryptedCredentialKeys.size(), &err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CBOR";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!cborArrayGetBStr(cbor.get(), 0, storageKey_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!cborArrayGetBStr(cbor.get(), 1, credentialPrivKey_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    // hexdump("storageKey", storageKey_);
    // hexdump("credentialPrivKey", credentialPrivKey_);

    // TODO: check/calculate failedAccessControlProfileIds

    requestCountsRemaining_ = args.requestCounts;
    currentNameSpace_ = "";

    _hidl_cb(ResultCode::OK, {});
    return Void();
}

Return<ResultCode> IdentityCredential::startRetrieveEntryValue(
    const hidl_string& nameSpace, const hidl_string& name,
    const hidl_vec<uint8_t>& accessControlProfileIds) {
    LOG(INFO) << "Entering " << __FUNCTION__;

    if (nameSpace.empty()) {
        LOG(ERROR) << "Name space cannot be empty";
        return ResultCode::FAILED;
    }

    if (requestCountsRemaining_.size() == 0) {
        LOG(ERROR) << "No more name spaces left to go through";
        return ResultCode::FAILED;
    }

    if (currentNameSpace_ == "") {
        // First call.
        currentNameSpace_ = nameSpace;
    }

    if (nameSpace == currentNameSpace_) {
        // Same namespace.
        if (requestCountsRemaining_[0] == 0) {
            LOG(ERROR) << "No more entries to be retrieved in current name space";
            return ResultCode::FAILED;
        }
        requestCountsRemaining_[0] -= 1;
    } else {
        // New namespace.
        if (requestCountsRemaining_[0] != 0) {
            LOG(ERROR) << "Moved to new name space but " << requestCountsRemaining_[0]
                       << " entries need to be retrieved in current name space";
            return ResultCode::FAILED;
        }
        requestCountsRemaining_.erase(requestCountsRemaining_.begin());
        currentNameSpace_ = nameSpace;
    }

    if (!entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
                                   entryAdditionalData_)) {
        return ResultCode::FAILED;
    }
    // hexdump("entryAdditionalData_", entryAdditionalData_);

    return ResultCode::OK;
}

Return<void> IdentityCredential::retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                                    retrieveEntryValue_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    EntryValue value;

    vector<uint8_t> plaintextValueCbor;
    if (!decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_,
                          plaintextValueCbor)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    hexdump("plaintextValueCbor", plaintextValueCbor);

    cn_cbor_errback err;
    auto cbor =
        CnCborPtr(cn_cbor_decode(plaintextValueCbor.data(), plaintextValueCbor.size(), &err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CBOR";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    switch (cbor.get()->type) {
        case CN_CBOR_TRUE:
            value.booleanValue(true);
            break;

        case CN_CBOR_FALSE:
            value.booleanValue(false);
            break;

        case CN_CBOR_UINT:
            value.integer(cbor.get()->v.uint);
            break;

        case CN_CBOR_INT:
            value.integer(cbor.get()->v.sint);
            break;

        case CN_CBOR_BYTES: {
            vector<uint8_t> data;
            data.resize(cbor.get()->length);
            memcpy(data.data(), cbor.get()->v.bytes, data.size());
            value.byteString(data);
        } break;

        case CN_CBOR_TEXT:
            value.textString(std::string(cbor.get()->v.str, cbor.get()->length));
            break;

        default:
            LOG(ERROR) << "Unexpected CBOR type " << cbor.get()->type;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
    }

    _hidl_cb(ResultCode::OK, value);
    return Void();
}

Return<void> IdentityCredential::finishRetrieval(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<uint8_t>& previousAuditSignatureHash,
    finishRetrieval_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    AuditLogEntry auditLogEntry;
    _hidl_cb(ResultCode::FAILED, {}, auditLogEntry);
    return Void();
}

Return<void> IdentityCredential::generateSigningKeyPair(KeyType keyType,
                                                        generateSigningKeyPair_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    _hidl_cb(ResultCode::FAILED, {}, {});
    return Void();
}

Return<ResultCode> IdentityCredential::provisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<hidl_vec<uint8_t>>& signingKeyData) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    return ResultCode::FAILED;
}

Return<void> IdentityCredential::getDirectAccessSigningKeyPairStatus(
    IIdentityCredential::getDirectAccessSigningKeyPairStatus_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    _hidl_cb(ResultCode::FAILED, {}, 0);
    return Void();
}

Return<ResultCode> IdentityCredential::deprovisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    return ResultCode::FAILED;
}

Return<ResultCode> IdentityCredential::configureDirectAccessPermissions(
    const hidl_vec<hidl_string>& itemsAllowedForDirectAccess) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    return ResultCode::FAILED;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
