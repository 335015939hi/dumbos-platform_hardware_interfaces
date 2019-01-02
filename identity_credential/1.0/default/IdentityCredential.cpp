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
#include "IdentityCredentialStore.h"

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
using android::hardware::identity_credential::support::cborMapPutStringValue;
using android::hardware::identity_credential::support::CnCborPtr;
using android::hardware::identity_credential::support::entryCreateAdditionalData;

// Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredential follow.

Return<void> IdentityCredential::deleteCredential(deleteCredential_cb _hidl_cb) {
    _hidl_cb(ResultCode::FAILED, {});
    return Void();
}

Return<void> IdentityCredential::createEphemeralKeyPair(KeyType keyType,
                                                        createEphemeralKeyPair_cb _hidl_cb) {
    _hidl_cb({});
    return Void();
}

bool IdentityCredential::initialize() {
    cn_cbor_errback err;

    auto dataCbor = CnCborPtr(cn_cbor_decode(credentialData_.data(), credentialData_.size(), &err));
    if (dataCbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CredentialData CBOR in decryptedCredentialKeys";
        return false;
    }
    if (!support::cborArrayGetString(dataCbor.get(), 0, docType_)) {
        return false;
    }
    if (!support::cborArrayGetBool(dataCbor.get(), 1, testCredential_)) {
        return false;
    }
    vector<uint8_t> credentialBlob = credentialData_;
    if (!support::cborArrayGetBStr(dataCbor.get(), 2, credentialBlob)) {
        return false;
    }

    vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    vector<uint8_t> decryptedCredentialKeys;
    if (!decryptAes128Gcm(getTestHardwareBoundKey(), credentialBlob, docTypeVec,
                          decryptedCredentialKeys)) {
        return false;
    }

    auto cbor = CnCborPtr(
        cn_cbor_decode(decryptedCredentialKeys.data(), decryptedCredentialKeys.size(), &err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CredentialBlob CBOR in decryptedCredentialKeys";
        return false;
    }
    if (!cborArrayGetBStr(cbor.get(), 0, storageKey_)) {
        return false;
    }
    if (!cborArrayGetBStr(cbor.get(), 1, credentialPrivKey_)) {
        return false;
    }

    return true;
}

Return<ResultCode> IdentityCredential::startRetrieval(const StartRetrievalArguments& args) {
    // TODO: check authToken

    // TODO: check/validate that readerSignature signs requestData (reader
    // public key is from one of the access control profiles).

    vector<uint8_t> encodedSessionTranscript;
    // If non-empty, |requestData| must be a valid CBOR map and it must contain
    // a key with the name "SessionTranscript". If present, put this value in
    // |AuthenticatedDataCBor_|
    cn_cbor_errback err;
    if (args.requestData.size() > 0) {
        auto requestDataCbor =
            CnCborPtr(cn_cbor_decode(args.requestData.data(), args.requestData.size(), &err));
        if (requestDataCbor.get() == nullptr) {
            LOG(ERROR) << "Error decoding CBOR in requestData";
            return ResultCode::FAILED;
        }
        if (requestDataCbor->type != CN_CBOR_MAP) {
            LOG(ERROR) << "requestDataCbor is not a map";
            return ResultCode::FAILED;
        }
        auto sessionTranscriptCbor =
            CnCborPtr(cn_cbor_mapget_string(requestDataCbor.get(), "SessionTranscript"));
        if (sessionTranscriptCbor.get() != nullptr) {
            if (!support::cborEncode(sessionTranscriptCbor.get(), encodedSessionTranscript)) {
                return ResultCode::FAILED;
            }
        }
    }
    authenticatedDataBuilder_.reset(docType_, encodedSessionTranscript);

    requestCountsRemaining_ = args.requestCounts;
    currentNameSpace_ = "";

    requestData_ = args.requestData;

    return ResultCode::OK;
}

Return<ResultCode> IdentityCredential::startRetrieveEntryValue(
    const hidl_string& nameSpace, const hidl_string& name, uint32_t entrySize,
    const hidl_vec<uint8_t>& accessControlProfileIds) {
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

    currentName_ = name;
    entryRemainingBytes_ = entrySize;
    entryBStrValue_.resize(0);
    entryStrValue_.resize(0);

    return ResultCode::OK;
}

Return<void> IdentityCredential::retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                                    retrieveEntryValue_cb _hidl_cb) {
    EntryValue value;

    vector<uint8_t> plaintextValueCbor;
    if (!decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_,
                          plaintextValueCbor)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    cn_cbor_errback err;
    auto cbor =
        CnCborPtr(cn_cbor_decode(plaintextValueCbor.data(), plaintextValueCbor.size(), &err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CBOR";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    ssize_t chunkSize = -1;
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
            chunkSize = data.size();
            entryBStrValue_.insert(entryBStrValue_.end(), data.begin(), data.end());
        } break;

        case CN_CBOR_TEXT: {
            std::string str = std::string(cbor.get()->v.str, cbor.get()->length);
            value.textString(str);
            chunkSize = str.size();
            entryStrValue_.append(str);
        } break;

        default:
            LOG(ERROR) << "Unexpected CBOR type " << cbor.get()->type;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
    }

    if (chunkSize < 0) {
        // This is the case where the value is not a tstr/bstr... make sure that
        // that 0 was passed as |entrySize| in startRetrieveEntryValue().
        if (entryRemainingBytes_ != 0) {
            LOG(ERROR) << "Remaining bytes is 0";
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
    } else {
        // value is a tstr/bstr
        if (size_t(chunkSize) > IdentityCredentialStore::kGcmChunkSize) {
            LOG(ERROR) << "Retrieved chunk of size " << chunkSize
                       << " is bigger than kGcmChunkSize which is "
                       << IdentityCredentialStore::kGcmChunkSize;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
        if (size_t(chunkSize) > entryRemainingBytes_) {
            LOG(ERROR) << "Retrieved chunk of size " << chunkSize
                       << " is bigger than remaining space of size " << entryRemainingBytes_;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
        entryRemainingBytes_ -= chunkSize;
        if (entryRemainingBytes_ > 0) {
            if (size_t(chunkSize) != IdentityCredentialStore::kGcmChunkSize) {
                LOG(ERROR) << "Retrieved non-final chunk of size " << chunkSize
                           << " but expected kGcmChunkSize which is "
                           << IdentityCredentialStore::kGcmChunkSize;
                _hidl_cb(ResultCode::FAILED, {});
                return Void();
            }
        }
    }

    if (entryRemainingBytes_ == 0) {
        // For bstr/tstr, use the full concatenated value instead of the passed-in value.
        EntryValue valueToWrite = value;
        switch (value.getDiscriminator()) {
            case EntryValue::hidl_discriminator::textString:
                valueToWrite.textString(entryStrValue_);
                break;
            case EntryValue::hidl_discriminator::byteString:
                valueToWrite.byteString(entryBStrValue_);
                break;
            default:
                // Do nothing.
                break;
        }
        authenticatedDataBuilder_.addDataItem(currentNameSpace_, currentName_, valueToWrite);
    }

    _hidl_cb(ResultCode::OK, value);
    return Void();
}

Return<void> IdentityCredential::finishRetrieval(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<uint8_t>& previousAuditSignatureHash,
    finishRetrieval_cb _hidl_cb) {
    AuditLogEntry auditLogEntry;
    vector<uint8_t> signature;

    // The case when no signing key is provided is not very interesting
    if (signingKeyBlob.size() == 0) {
        _hidl_cb(ResultCode::FAILED, {}, auditLogEntry);
        return Void();
    }

    vector<uint8_t> signingKey;
    // TODO: what to use for auth data?
    if (!decryptAes128Gcm(storageKey_, signingKeyBlob, {}, signingKey)) {
        _hidl_cb(ResultCode::FAILED, {}, auditLogEntry);
        return Void();
    }

    std::vector<uint8_t> encodedCbor;
    if (!authenticatedDataBuilder_.getEncodedCbor(encodedCbor)) {
        _hidl_cb(ResultCode::FAILED, {}, auditLogEntry);
        return Void();
    }

    if (!support::signEcDsa(signingKey, encodedCbor, signature)) {
        _hidl_cb(ResultCode::FAILED, {}, auditLogEntry);
        return Void();
    }

    _hidl_cb(ResultCode::OK, signature, auditLogEntry);
    return Void();
}

Return<void> IdentityCredential::generateSigningKeyPair(KeyType keyType,
                                                        generateSigningKeyPair_cb _hidl_cb) {
    if (keyType != KeyType::EC_NIST_P_256) {
        LOG(ERROR) << "Unexpected keyType\n";
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> certificate;
    vector<uint8_t> signingKey;
    if (!support::createEcKeyAndAttestationChain(signingKey, certificate)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> nonce;
    if (!getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    vector<uint8_t> encryptedSigningKey;
    // TODO: what to use for auth data?
    if (!encryptAes128Gcm(storageKey_, nonce, signingKey, {}, encryptedSigningKey)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    _hidl_cb(ResultCode::OK, encryptedSigningKey, certificate);
    return Void();
}

Return<ResultCode> IdentityCredential::provisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<hidl_vec<uint8_t>>& signingKeyData) {
    return ResultCode::FAILED;
}

Return<void> IdentityCredential::getDirectAccessSigningKeyPairStatus(
    IIdentityCredential::getDirectAccessSigningKeyPairStatus_cb _hidl_cb) {
    _hidl_cb(ResultCode::FAILED, {}, 0);
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
