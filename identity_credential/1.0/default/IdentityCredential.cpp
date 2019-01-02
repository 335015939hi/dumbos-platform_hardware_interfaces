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

#include <string.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using std::string;
using std::vector;

// Methods from ::android::hardware::identity_credential::V1_0::IIdentityCredential follow.

Return<void> IdentityCredential::deleteCredential(deleteCredential_cb _hidl_cb) {
    _hidl_cb(ResultCode::FAILED, {});
    return Void();
}

Return<void> IdentityCredential::createEphemeralKeyPair(createEphemeralKeyPair_cb _hidl_cb) {
    vector<uint8_t> keyPair;
    if (!support::createEcKeyPair(keyPair)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    // Stash public key of this key-pair for later check in startRetrieval().
    if (!support::ecKeyPairGetPublicKey(keyPair, ephemeralPublicKey_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    _hidl_cb(ResultCode::OK, keyPair);
    return Void();
}

bool IdentityCredential::initialize() {
    cn_cbor_errback err;

    auto dataCbor =
        support::CnCborPtr(cn_cbor_decode(credentialData_.data(), credentialData_.size(), &err));
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

    vector<uint8_t> hardwareBoundKey;
    if (testCredential_) {
        hardwareBoundKey = support::getTestHardwareBoundKey();
    } else {
        hardwareBoundKey = support::getHardwareBoundKey();
    }

    vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    vector<uint8_t> decryptedCredentialKeys;
    if (!support::decryptAes128Gcm(hardwareBoundKey, credentialBlob, docTypeVec,
                                   decryptedCredentialKeys)) {
        return false;
    }

    auto cbor = support::CnCborPtr(
        cn_cbor_decode(decryptedCredentialKeys.data(), decryptedCredentialKeys.size(), &err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error decoding CredentialBlob CBOR in decryptedCredentialKeys";
        return false;
    }
    if (!support::cborArrayGetBStr(cbor.get(), 0, storageKey_)) {
        return false;
    }
    if (!support::cborArrayGetBStr(cbor.get(), 1, credentialPrivKey_)) {
        return false;
    }

    return true;
}

Return<ResultCode> IdentityCredential::startRetrieval(const StartRetrievalArguments& args) {
    // Find readerAuthPubKey (if it exists) and bail if there's more than one.
    vector<uint8_t> readerCertificate;
    for (const auto& profile : args.accessControlProfiles) {
        if (profile.readerAuthPubKey.size() > 0) {
            if (readerCertificate.size() > 0) {
                LOG(ERROR) << "At most one of the profiles may contain non-empty readerCertificate";
                return ResultCode::FAILED;
            }
            readerCertificate = profile.readerAuthPubKey;
        }
    }

    // If there is a readerAuthKey set, validate that it was used to sign the
    // entirety of the requestData.
    if (args.readerSignature.size() > 0) {
        if (!support::checkEcDsaSignature(support::sha256(args.requestData), args.readerSignature,
                                          readerCertificate)) {
            LOG(ERROR) << "readerSignature check failed";
            return ResultCode::FAILED;
        }
    }

    // Validate the passed-in |authToken|
    if (!support::validateAuthToken(args.authToken)) {
        LOG(ERROR) << "authToken validation failed";
        return ResultCode::FAILED;
    }

    // requestData: If non-empty, contains request data that is signed by the
    // reader.  The content can be defined in the way appropriate for the
    // credential, but there are four requirements that must be met to work with
    // this HAL:
    vector<uint8_t> encodedSessionTranscript;
    if (args.requestData.size() > 0) {
        cn_cbor_errback err;

        // 1. The content must be a CBOR-encoded structure.
        auto requestDataCbor = support::CnCborPtr(
            cn_cbor_decode(args.requestData.data(), args.requestData.size(), &err));
        if (requestDataCbor.get() == nullptr) {
            LOG(ERROR) << "Error decoding CBOR in requestData";
            return ResultCode::FAILED;
        }

        // 2. The CBOR structure must be a map.
        if (requestDataCbor->type != CN_CBOR_MAP) {
            LOG(ERROR) << "requestData is not a CBOR map";
            return ResultCode::FAILED;
        }

        // 3. The map must contain a key "SessionTranscript" whose value must be a CBOR data
        // structure
        //    that, somewhere contains a CBOR value of type bytestring, with the value of the
        //    ephemeral key created by createEphemeralKeyPair.
        cn_cbor* sessionTranscriptCbor =
            cn_cbor_mapget_string(requestDataCbor.get(), "SessionTranscript");
        if (sessionTranscriptCbor == nullptr) {
            LOG(ERROR) << "no SessionTranscript in requestData";
            return ResultCode::FAILED;
        }
        if (!support::cborHasBStr(sessionTranscriptCbor, ephemeralPublicKey_)) {
            LOG(ERROR) << "Did not find public part of the ephemeralKeyPair in SessionTranscript";
            return ResultCode::FAILED;
        }
        // -- HACK ALERT BEGIN: This hack is needed in order to encode a cn_cbor* which
        //                      is already part of a tree.
        cn_cbor* savedParent = sessionTranscriptCbor->parent;
        cn_cbor* savedNext = sessionTranscriptCbor->next;
        sessionTranscriptCbor->parent = nullptr;
        sessionTranscriptCbor->next = nullptr;
        // We need the CBOR-encoded SessionTranscript for later so encode it now.
        if (!support::cborEncode(sessionTranscriptCbor, encodedSessionTranscript)) {
            return ResultCode::FAILED;
        }
        sessionTranscriptCbor->next = savedNext;
        sessionTranscriptCbor->parent = savedParent;
        // -- HACK ALERT END

        // 4. The map must contain a key "Request" whose value is an array of maps, defined below.
        //
        //      DataReq = {
        //        ? "DocType" : DocType,
        //        + Namespace => DataItemNames
        //      }
        //      DocType = tstr
        //      Namespace = tstr
        //      DataItemNames = [ + tstr ]
        //
        // Here's an example of such a value satisfying these requirements:
        //
        //    {
        //      'SessionTranscript' : {
        //        'EphemeralPublicKey' : <some bytestring>
        //      },
        //      'Request' : [{
        //          'DocType' : 'org.iso.18013-5.2019.mdl',
        //          'PersonalData' : ['Last name', 'Birth date', 'First name', 'Home address'],
        //          'Image' : ['Portrait image']
        //        }, {
        //          'DocType' : 'com.android.identity_credential.example.library_card',
        //          'PersonalData' : ['Last name', 'First name'],
        //          'Image' : ['Portrait image']
        //        }]
        //    }
        //
        cn_cbor* arrayDataReqs = cn_cbor_mapget_string(requestDataCbor.get(), "Request");
        if (arrayDataReqs == nullptr) {
            LOG(ERROR) << "no DataReq array in requestData";
            return ResultCode::FAILED;
        }
        for (size_t n = 0; n < (size_t)arrayDataReqs->length; n++) {
            cn_cbor* item = cn_cbor_index(arrayDataReqs, n);
            if (item == nullptr) {
                LOG(ERROR) << "Error retrieving index " << n << " of Request array in requestData";
                return ResultCode::FAILED;
            }
            if (item->type != CN_CBOR_MAP) {
                LOG(ERROR) << "Index " << n << " of Request array in requestData is not a map";
                return ResultCode::FAILED;
            }
            if (item->length < 2) {
                LOG(ERROR) << "Number of items in map at index " << n
                           << " of Request array in requestData is " << item->length
                           << " and expected at least 2";
                return ResultCode::FAILED;
            }
            cn_cbor* itemKey = item->first_child;
            cn_cbor* itemValue = itemKey->next;
            if (itemKey->type != CN_CBOR_TEXT || itemValue->type != CN_CBOR_TEXT ||
                (size_t)itemKey->length != strlen("DocType") ||
                memcmp(itemKey->v.str, "DocType", strlen("DocType")) != 0) {
                LOG(ERROR) << "Expected first item in map to be str->str with key 'DocType' but "
                              "found other value";
                return ResultCode::FAILED;
            }
            string requestDocType = string(itemValue->v.str, itemValue->length);
            // If this is for a docType which isn't what the credential was
            // provisioned for, silently ignore it (we'll error out in
            // startRetrieveEntryValue() though).
            if (requestDocType != docType_) {
                LOG(INFO) << "Ignoring request for docType '" << requestDocType
                          << "' since this credential is for docType '" << docType_ << "'";
                continue;
            }
            // Now collect all DataItemNames for said DocType and store them so
            // they can be checked against in startRetrievalEntryValue().
            for (cn_cbor* mapIt = itemValue->next; mapIt != nullptr; mapIt = mapIt->next->next) {
                cn_cbor* mapKey = mapIt;
                cn_cbor* mapValue = mapIt->next;
                if (mapKey->type != CN_CBOR_TEXT || mapValue->type != CN_CBOR_ARRAY) {
                    LOG(ERROR)
                        << "Expected mapKey to be CN_CBOR_TEXT and mapValue to be CN_CBOR_ARRAY";
                    return ResultCode::FAILED;
                }
                string requestedNameSpace = string(mapKey->v.str, mapKey->length);
                vector<string> requestedKeys;
                for (size_t m = 0; m < (size_t)mapValue->length; m++) {
                    cn_cbor* dataItemName = cn_cbor_index(mapValue, m);
                    if (dataItemName->type != CN_CBOR_TEXT) {
                        LOG(ERROR) << "Expected dataItemName to be CN_CBOR_TEXT";
                        return ResultCode::FAILED;
                    }
                    requestedKeys.push_back(string(dataItemName->v.str, dataItemName->length));
                }
                requestedNameSpacesAndNames_[requestedNameSpace] = requestedKeys;
            }
        }
    }

    // Finally, validate all the access control profiles in the requestData.
    vector<uint8_t> sessionTranscriptDigest;
    validatedProfileIds_.clear();
    if (encodedSessionTranscript.size() > 0) {
        sessionTranscriptDigest = support::sha256(encodedSessionTranscript);
    }
    for (const auto& profile : args.accessControlProfiles) {
        if (!support::secureAccessControlProfileCheckMac(profile, storageKey_)) {
            LOG(ERROR) << "Error checking MAC for profile with id " << int(profile.id);
            return ResultCode::FAILED;
        }
        if (!support::checkAccess(profile, args.authToken, sessionTranscriptDigest)) {
            LOG(ERROR) << "authToken does not grant access to profile with id " << int(profile.id);
            return ResultCode::FAILED;
        }
        validatedProfileIds_.insert(profile.id);
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

    for (auto id : accessControlProfileIds) {
        if (validatedProfileIds_.find(id) == validatedProfileIds_.end()) {
            LOG(ERROR) << "Requested entry with unvalidated profile id " << id;
            return ResultCode::FAILED;
        }
    }

    // It's permissible to have an empty requestData... but if non-empty you can
    // only request what was specified in said requestData. Enforce that.
    if (requestData_.size() > 0) {
        const auto& it = requestedNameSpacesAndNames_.find(nameSpace);
        if (it == requestedNameSpacesAndNames_.end()) {
            LOG(ERROR) << "Name space '" << nameSpace << "' was not requested in startRetrieval";
            return ResultCode::FAILED;
        }
        const auto& dataItemNames = it->second;
        if (std::find(dataItemNames.begin(), dataItemNames.end(), name) == dataItemNames.end()) {
            LOG(ERROR) << "Data item name '" << name << "' in name space '" << nameSpace
                       << "' was not requested in startRetrieval";
            return ResultCode::FAILED;
        }
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

    if (!support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
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
    if (!support::decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_,
                                   plaintextValueCbor)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    cn_cbor_errback err;
    auto cbor = support::CnCborPtr(
        cn_cbor_decode(plaintextValueCbor.data(), plaintextValueCbor.size(), &err));
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
            string str = string(cbor.get()->v.str, cbor.get()->length);
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
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> signingKey;
    vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
    if (!support::decryptAes128Gcm(storageKey_, signingKeyBlob, docTypeAsBlob, signingKey)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> encodedCbor;
    if (!authenticatedDataBuilder_.getEncodedCbor(encodedCbor)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    if (!support::signEcDsa(signingKey, encodedCbor, signature)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> requestHash = support::sha256(requestData_);
    vector<uint8_t> responseHash = support::sha256(encodedCbor);
    vector<uint8_t> encodedAuditLogData;
    if (!support::buildAndEncodeAuditLogData(requestHash, responseHash, previousAuditSignatureHash,
                                             encodedAuditLogData)) {
        LOG(ERROR) << "Error building AuditLogData";
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    vector<uint8_t> auditLogSignature;
    if (!support::signEcDsa(credentialPrivKey_, encodedAuditLogData, auditLogSignature)) {
        LOG(ERROR) << "Error signing AuditLogData";
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    memcpy(auditLogEntry.requestHash.data(), requestHash.data(), 32);
    memcpy(auditLogEntry.responseHash.data(), responseHash.data(), 32);
    auditLogEntry.signature = auditLogSignature;

    _hidl_cb(ResultCode::OK, signature, auditLogEntry);
    return Void();
}

Return<void> IdentityCredential::generateSigningKeyPair(generateSigningKeyPair_cb _hidl_cb) {
    vector<uint8_t> signingKeyPKCS8;
    vector<uint8_t> signingPublicKey;
    vector<uint8_t> signingKey;
    vector<uint8_t> certificate;
    string serialDecimal = "0";  // TODO: set serial to something unique
    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential Reference Implementation";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;
    if (!support::createEcKeyPair(signingKeyPKCS8) ||
        !support::ecKeyPairGetPublicKey(signingKeyPKCS8, signingPublicKey) ||
        !support::ecKeyPairGetPrivateKey(signingKeyPKCS8, signingKey) ||
        !support::ecPublicKeyGenerateCertificate(signingPublicKey, credentialPrivKey_,
                                                 serialDecimal, issuer, subject, validityNotBefore,
                                                 validityNotAfter, certificate)) {
        LOG(ERROR) << "Error creating signingKey";
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    vector<uint8_t> encryptedSigningKey;
    vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
    if (!support::encryptAes128Gcm(storageKey_, nonce, signingKey, docTypeAsBlob,
                                   encryptedSigningKey)) {
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
