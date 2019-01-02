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
    _hidl_cb(support::result(ResultCode::FAILED, "TODO: Not yet specified / implemented"), {});
    return Void();
}

Return<void> IdentityCredential::createEphemeralKeyPair(createEphemeralKeyPair_cb _hidl_cb) {
    vector<uint8_t> keyPair;
    if (!support::createEcKeyPair(keyPair)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating ephemeral key pair"), {});
        return Void();
    }

    // Stash public key of this key-pair for later check in startRetrieval().
    if (!support::ecKeyPairGetPublicKey(keyPair, ephemeralPublicKey_)) {
        _hidl_cb(
            support::result(ResultCode::FAILED, "Error getting public part of ephemeral key pair"),
            {});
        return Void();
    }

    _hidl_cb(support::resultOK(), keyPair);
    return Void();
}

ResultCode IdentityCredential::initialize() {
    cn_cbor_errback err;

    auto dataCbor =
        support::CnCborPtr(cn_cbor_decode(credentialData_.data(), credentialData_.size(), &err));
    vector<uint8_t> encryptedCredentialKeys;
    if (dataCbor.get() == nullptr || !support::cborArrayGetString(dataCbor.get(), 0, docType_) ||
        !support::cborArrayGetBool(dataCbor.get(), 1, testCredential_) ||
        !support::cborArrayGetBStr(dataCbor.get(), 2, encryptedCredentialKeys)) {
        LOG(ERROR) << "Error decoding CredentialData CBOR";
        return ResultCode::INVALID_DATA;
    }

    vector<uint8_t> hardwareBoundKey;
    if (testCredential_) {
        hardwareBoundKey = support::getTestHardwareBoundKey();
    } else {
        hardwareBoundKey = support::getHardwareBoundKey();
    }

    vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    vector<uint8_t> decryptedCredentialKeys;
    if (!support::decryptAes128Gcm(hardwareBoundKey, encryptedCredentialKeys, docTypeVec,
                                   decryptedCredentialKeys)) {
        LOG(ERROR) << "Error decrypting CredentialKeys";
        return ResultCode::INVALID_DATA;
    }

    auto cbor = support::CnCborPtr(
        cn_cbor_decode(decryptedCredentialKeys.data(), decryptedCredentialKeys.size(), &err));
    if (cbor.get() == nullptr || !support::cborArrayGetBStr(cbor.get(), 0, storageKey_) ||
        !support::cborArrayGetBStr(cbor.get(), 1, credentialPrivKey_)) {
        LOG(ERROR) << "Error decoding CredentialKeys CBOR";
        return ResultCode::INVALID_DATA;
    }

    return ResultCode::OK;
}

Return<void> IdentityCredential::startRetrieval(const StartRetrievalArguments& args,
                                                startRetrieval_cb _hidl_cb) {
    // Find readerCertificate (if it exists) and bail if there's more than one.
    vector<uint8_t> readerCertificate;
    for (const auto& profile : args.accessControlProfiles) {
        if (profile.readerCertificate.size() > 0) {
            if (readerCertificate.size() > 0) {
                _hidl_cb(support::result(
                    ResultCode::INVALID_DATA,
                    "At most one of the profiles may contain non-empty readerCertificate"));
                return Void();
            }
            readerCertificate = profile.readerCertificate;
        }
    }

    // If there is a readerAuthKey set, validate that it was used to sign the
    // entirety of the requestData.
    if (args.readerSignature.size() > 0) {
        if (!support::checkEcDsaSignature(support::sha256(args.requestData), args.readerSignature,
                                          readerCertificate)) {
            _hidl_cb(support::result(ResultCode::READER_SIGNATURE_CHECK_FAILED,
                                     "readerSignature check failed"));
            return Void();
        }
    }

    // Validate the passed-in |authToken|
    if (!support::validateAuthToken(args.authToken)) {
        _hidl_cb(support::result(ResultCode::INVALID_AUTH_TOKEN, "authToken validation failed"));
        return Void();
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
            _hidl_cb(
                support::result(ResultCode::INVALID_DATA, "Error decoding CBOR in requestData"));
            return Void();
        }

        // 2. The CBOR structure must be a map.
        if (requestDataCbor->type != CN_CBOR_MAP) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "requestData is not a CBOR map"));
            return Void();
        }

        // 3. The map must contain a key "SessionTranscript" whose value must be a CBOR data
        // structure
        //    that, somewhere contains a CBOR value of type bytestring, with the value of the
        //    ephemeral key created by createEphemeralKeyPair.
        cn_cbor* sessionTranscriptCbor =
            cn_cbor_mapget_string(requestDataCbor.get(), "SessionTranscript");
        if (sessionTranscriptCbor == nullptr) {
            _hidl_cb(
                support::result(ResultCode::INVALID_DATA, "no SessionTranscript in requestData"));
            return Void();
        }
        if (!support::cborHasBStr(sessionTranscriptCbor, ephemeralPublicKey_)) {
            _hidl_cb(support::result(
                ResultCode::INVALID_DATA,
                "Did not find public part of the ephemeralKeyPair in SessionTranscript"));
            return Void();
        }
        // -- HACK ALERT BEGIN: This hack is needed in order to encode a cn_cbor* which
        //                      is already part of a tree.
        cn_cbor* savedParent = sessionTranscriptCbor->parent;
        cn_cbor* savedNext = sessionTranscriptCbor->next;
        sessionTranscriptCbor->parent = nullptr;
        sessionTranscriptCbor->next = nullptr;
        // We need the CBOR-encoded SessionTranscript for later so encode it now.
        if (!support::cborEncode(sessionTranscriptCbor, encodedSessionTranscript)) {
            _hidl_cb(support::result(ResultCode::FAILED, "Error reencoding SessionTranscript"));
            return Void();
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
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "no DataReq array in requestData"));
            return Void();
        }
        for (size_t n = 0; n < (size_t)arrayDataReqs->length; n++) {
            cn_cbor* item = cn_cbor_index(arrayDataReqs, n);
            if (item == nullptr) {
                _hidl_cb(support::result(
                    ResultCode::INVALID_DATA,
                    "Error retrieving index %zd of Request array in requestData", n));
                return Void();
            }
            if (item->type != CN_CBOR_MAP) {
                _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                         "Index %zd of Request array in requestData is not a map",
                                         n));
                return Void();
            }
            if (item->length < 2) {
                _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                         "Number of items in map at index %zd "
                                         " of Request array in requestData is %zd"
                                         " and expected at least 2",
                                         n, (size_t)item->length));
                return Void();
            }
            cn_cbor* itemKey = item->first_child;
            cn_cbor* itemValue = itemKey->next;
            if (itemKey->type != CN_CBOR_TEXT || itemValue->type != CN_CBOR_TEXT ||
                (size_t)itemKey->length != strlen("DocType") ||
                memcmp(itemKey->v.str, "DocType", strlen("DocType")) != 0) {
                _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                         "Expected first item in map to be str->str with key "
                                         "'DocType' but found other value"));
                return Void();
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
                    _hidl_cb(support::result(
                        ResultCode::INVALID_DATA,
                        "Expected mapKey to be CN_CBOR_TEXT and mapValue to be CN_CBOR_ARRAY"));
                    return Void();
                }
                string requestedNameSpace = string(mapKey->v.str, mapKey->length);
                vector<string> requestedKeys;
                for (size_t m = 0; m < (size_t)mapValue->length; m++) {
                    cn_cbor* dataItemName = cn_cbor_index(mapValue, m);
                    if (dataItemName->type != CN_CBOR_TEXT) {
                        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                                 "Expected dataItemName to be CN_CBOR_TEXT"));
                        return Void();
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
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Error checking MAC for profile with id %d", int(profile.id)));
            return Void();
        }
        if (!support::checkAccess(profile, args.authToken, sessionTranscriptDigest)) {
            _hidl_cb(support::result(ResultCode::ACCESS_DENIED,
                                     "authToken does not grant access to profile with id %d",
                                     int(profile.id)));
            return Void();
        }
        validatedProfileIds_.insert(profile.id);
    }

    authenticatedDataBuilder_.reset(docType_, encodedSessionTranscript);
    requestCountsRemaining_ = args.requestCounts;
    currentNameSpace_ = "";

    requestData_ = args.requestData;

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> IdentityCredential::startRetrieveEntryValue(
    const hidl_string& nameSpace, const hidl_string& name, uint32_t entrySize,
    const hidl_vec<uint8_t>& accessControlProfileIds, startRetrieveEntryValue_cb _hidl_cb) {
    if (name.empty()) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Name cannot be empty"));
        return Void();
    }
    if (nameSpace.empty()) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Name space cannot be empty"));
        return Void();
    }

    for (auto id : accessControlProfileIds) {
        if (validatedProfileIds_.find(id) == validatedProfileIds_.end()) {
            _hidl_cb(support::result(ResultCode::ACCESS_DENIED,
                                     "Requested entry with unvalidated profile id %d", (int(id))));
            return Void();
        }
    }

    // It's permissible to have an empty requestData... but if non-empty you can
    // only request what was specified in said requestData. Enforce that.
    if (requestData_.size() > 0) {
        const auto& it = requestedNameSpacesAndNames_.find(nameSpace);
        if (it == requestedNameSpacesAndNames_.end()) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Name space '%s' was not requested in startRetrieval",
                                     nameSpace.c_str()));
            return Void();
        }
        const auto& dataItemNames = it->second;
        if (std::find(dataItemNames.begin(), dataItemNames.end(), name) == dataItemNames.end()) {
            _hidl_cb(support::result(
                ResultCode::INVALID_DATA,
                "Data item name '%s' in name space '%s' was not requested in startRetrieval",
                name.c_str(), nameSpace.c_str()));
            return Void();
        }
    }

    if (requestCountsRemaining_.size() == 0) {
        _hidl_cb(
            support::result(ResultCode::INVALID_DATA, "No more name spaces left to go through"));
        return Void();
    }

    if (currentNameSpace_ == "") {
        // First call.
        currentNameSpace_ = nameSpace;
    }

    if (nameSpace == currentNameSpace_) {
        // Same namespace.
        if (requestCountsRemaining_[0] == 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "No more entries to be retrieved in current name space"));
            return Void();
        }
        requestCountsRemaining_[0] -= 1;
    } else {
        // New namespace.
        if (requestCountsRemaining_[0] != 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Moved to new name space but %d entries need to be retrieved "
                                     "in current name space",
                                     int(requestCountsRemaining_[0])));
            return Void();
        }
        requestCountsRemaining_.erase(requestCountsRemaining_.begin());
        currentNameSpace_ = nameSpace;
    }

    if (!support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
                                            entryAdditionalData_)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating AdditionalData"));
        return Void();
    }

    currentName_ = name;
    entryRemainingBytes_ = entrySize;
    entryBStrValue_.resize(0);
    entryStrValue_.resize(0);

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> IdentityCredential::retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                                    retrieveEntryValue_cb _hidl_cb) {
    EntryValue value;

    vector<uint8_t> plaintextValueCbor;
    if (!support::decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_,
                                   plaintextValueCbor)) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error decrypting data"), {});
        return Void();
    }

    cn_cbor_errback err;
    auto cbor = support::CnCborPtr(
        cn_cbor_decode(plaintextValueCbor.data(), plaintextValueCbor.size(), &err));
    if (cbor.get() == nullptr) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error decoding CBOR"), {});
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
            vector<uint8_t> data;
            data.resize(cbor.get()->length);
            memcpy(data.data(), cbor.get()->v.str, data.size());
            value.textString(data);
            chunkSize = data.size();
            entryStrValue_.insert(entryStrValue_.end(), data.begin(), data.end());
        } break;

        default:
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Unexpected CBOR type %d",
                                     int(cbor.get()->type)),
                     {});
            return Void();
    }

    if (chunkSize < 0) {
        // This is the case where the value is not a tstr/bstr... make sure that
        // that 0 was passed as |entrySize| in startRetrieveEntryValue().
        if (entryRemainingBytes_ != 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Remaining bytes is 0"), {});
            return Void();
        }
    } else {
        // value is a tstr/bstr
        if (size_t(chunkSize) > IdentityCredentialStore::kGcmChunkSize) {
            _hidl_cb(support::result(
                         ResultCode::INVALID_DATA,
                         "Retrieved chunk of size %zd is bigger than kGcmChunkSize which is %zd",
                         size_t(chunkSize), IdentityCredentialStore::kGcmChunkSize),
                     {});
            return Void();
        }
        if (size_t(chunkSize) > entryRemainingBytes_) {
            LOG(ERROR) << "Retrieved chunk of size " << chunkSize
                       << " is bigger than remaining space of size " << entryRemainingBytes_;
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Retrieved chunk of size %zd is bigger than remaining space "
                                     "of size %zd",
                                     size_t(chunkSize), entryRemainingBytes_),
                     {});
            return Void();
        }
        entryRemainingBytes_ -= chunkSize;
        if (entryRemainingBytes_ > 0) {
            if (size_t(chunkSize) != IdentityCredentialStore::kGcmChunkSize) {
                _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                         "Retrieved non-final chunk of size %zd but expected "
                                         "kGcmChunkSize which is %zd",
                                         size_t(chunkSize), IdentityCredentialStore::kGcmChunkSize),
                         {});
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

    _hidl_cb(support::resultOK(), value);
    return Void();
}

Return<void> IdentityCredential::finishRetrieval(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<uint8_t>& previousAuditSignatureHash,
    finishRetrieval_cb _hidl_cb) {
    AuditLogEntry auditLogEntry;

    vector<uint8_t> encodedCbor;
    if (!authenticatedDataBuilder_.getEncodedCbor(encodedCbor)) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error building AuthenticatedData"), {},
                 {});
        return Void();
    }

    // If there's no signing key, we return the empty signature.
    vector<uint8_t> signature;
    if (signingKeyBlob.size() > 0) {
        vector<uint8_t> signingKey;
        vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
        if (!support::decryptAes128Gcm(storageKey_, signingKeyBlob, docTypeAsBlob, signingKey)) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error decrypting signingKeyBlob"),
                     {}, {});
            return Void();
        }

        if (!support::signEcDsa(signingKey, encodedCbor, signature)) {
            _hidl_cb(support::result(ResultCode::FAILED, "Error signing AuthenticatedData"), {},
                     {});
            return Void();
        }
    }

    if (previousAuditSignatureHash.size() != 32) {
        _hidl_cb(
            support::result(ResultCode::INVALID_DATA, "Unexpected previousAuditSignatureHash size"),
            {}, {});
        return Void();
    }
    vector<uint8_t> requestHash = support::sha256(requestData_);
    vector<uint8_t> responseHash = support::sha256(encodedCbor);
    vector<uint8_t> encodedAuditLogData;
    if (!support::buildAndEncodeAuditLogData(requestHash, responseHash, previousAuditSignatureHash,
                                             encodedAuditLogData)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error building AuditLogData"), {}, {});
        return Void();
    }
    vector<uint8_t> auditLogSignature;
    if (!support::signEcDsa(credentialPrivKey_, encodedAuditLogData, auditLogSignature)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error signing AuditLogData"), {}, {});
        return Void();
    }
    memcpy(auditLogEntry.requestHash.data(), requestHash.data(), 32);
    memcpy(auditLogEntry.responseHash.data(), responseHash.data(), 32);
    auditLogEntry.signature = auditLogSignature;

    _hidl_cb(support::resultOK(), signature, auditLogEntry);
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
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating signingKey"), {}, {});
        return Void();
    }

    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error getting random"), {}, {});
        return Void();
    }
    vector<uint8_t> encryptedSigningKey;
    vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
    if (!support::encryptAes128Gcm(storageKey_, nonce, signingKey, docTypeAsBlob,
                                   encryptedSigningKey)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error encrypting signingKey"), {}, {});
        return Void();
    }

    _hidl_cb(support::resultOK(), encryptedSigningKey, certificate);
    return Void();
}

Return<void> IdentityCredential::provisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob, const hidl_vec<hidl_vec<uint8_t>>& signingKeyData,
    provisionDirectAccessSigningKeyPair_cb _hidl_cb) {
    _hidl_cb(support::result(ResultCode::UNSUPPORTED_OPERATION,
                             "Not implemented by this implementation"));
    return Void();
}

Return<void> IdentityCredential::getDirectAccessSigningKeyPairStatus(
    IIdentityCredential::getDirectAccessSigningKeyPairStatus_cb _hidl_cb) {
    _hidl_cb(support::result(ResultCode::UNSUPPORTED_OPERATION,
                             "Not implemented by this implementation"),
             {}, 0);
    return Void();
}

Return<void> IdentityCredential::deprovisionDirectAccessSigningKeyPair(
    const hidl_vec<uint8_t>& signingKeyBlob, deprovisionDirectAccessSigningKeyPair_cb _hidl_cb) {
    _hidl_cb(support::result(ResultCode::UNSUPPORTED_OPERATION,
                             "Not implemented by this implementation"));
    return Void();
}

Return<void> IdentityCredential::configureDirectAccessPermissions(
    const hidl_vec<hidl_string>& itemsAllowedForDirectAccess,
    configureDirectAccessPermissions_cb _hidl_cb) {
    _hidl_cb(support::result(ResultCode::UNSUPPORTED_OPERATION,
                             "Not implemented by this implementation"));
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
