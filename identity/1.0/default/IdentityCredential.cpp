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

#define LOG_TAG "IdentityCredential"

#include "IdentityCredential.h"
#include "IdentityCredentialStore.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <string.h>

#include <android-base/logging.h>

#include <cppbor.h>
#include <cppbor_parse.h>

namespace android {
namespace hardware {
namespace identity {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::identity::V1_0::IIdentityCredential follow.

Return<void> IdentityCredential::deleteCredential(deleteCredential_cb _hidl_cb) {
    cppbor::Array array;
    array.add("ProofOfDeletion");
    array.add(docType_);
    array.add(testCredential_);
    vector<uint8_t> proofOfDeletion = array.encode();

    vector<uint8_t> proofOfDeletionSignature;
    if (!support::coseSignEcDsa(credentialPrivKey_,
                                proofOfDeletion,  // payload
                                {},               // additionalData
                                {},               // certificateChain
                                proofOfDeletionSignature)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error signing data"), {});
        return Void();
    }

    _hidl_cb(support::resultOK(), proofOfDeletionSignature);
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
        _hidl_cb(support::result(ResultCode::FAILED,
                                 "Error getting public part of ephemeral key pair"),
                 {});
        return Void();
    }

    _hidl_cb(support::resultOK(), keyPair);
    return Void();
}

ResultCode IdentityCredential::initialize() {
    auto [item, _, message] = cppbor::parse(credentialData_);
    if (item == nullptr) {
        LOG(ERROR) << "CredentialData is not valid CBOR: " << message;
        return ResultCode::INVALID_DATA;
    }

    const cppbor::Array* arrayItem = item->asArray();
    if (arrayItem == nullptr || arrayItem->size() != 3) {
        LOG(ERROR) << "CredentialData is not an array with three elements";
        return ResultCode::INVALID_DATA;
    }

    const cppbor::Tstr* docTypeItem = (*arrayItem)[0]->asTstr();
    const cppbor::Bool* testCredentialItem =
            ((*arrayItem)[1]->asSimple() != nullptr ? ((*arrayItem)[1]->asSimple()->asBool())
                                                    : nullptr);
    const cppbor::Bstr* encryptedCredentialKeysItem = (*arrayItem)[2]->asBstr();
    if (docTypeItem == nullptr || testCredentialItem == nullptr ||
        encryptedCredentialKeysItem == nullptr) {
        LOG(ERROR) << "CredentialData unexpected item types";
        return ResultCode::INVALID_DATA;
    }

    docType_ = docTypeItem->value();
    testCredential_ = testCredentialItem->value();

    vector<uint8_t> hardwareBoundKey;
    if (testCredential_) {
        hardwareBoundKey = support::getTestHardwareBoundKey();
    } else {
        hardwareBoundKey = support::getHardwareBoundKey();
    }

    const vector<uint8_t>& encryptedCredentialKeys = encryptedCredentialKeysItem->value();
    const vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    vector<uint8_t> decryptedCredentialKeys;
    if (!support::decryptAes128Gcm(hardwareBoundKey, encryptedCredentialKeys, docTypeVec,
                                   decryptedCredentialKeys)) {
        LOG(ERROR) << "Error decrypting CredentialKeys";
        return ResultCode::INVALID_DATA;
    }

    auto [dckItem, dckPos, dckMessage] = cppbor::parse(decryptedCredentialKeys);
    if (dckItem == nullptr) {
        LOG(ERROR) << "Decrypted CredentialKeys is not valid CBOR: " << dckMessage;
        return ResultCode::INVALID_DATA;
    }
    const cppbor::Array* dckArrayItem = dckItem->asArray();
    if (dckArrayItem == nullptr || dckArrayItem->size() != 2) {
        LOG(ERROR) << "Decrypted CredentialKeys is not an array with two elements";
        return ResultCode::INVALID_DATA;
    }
    const cppbor::Bstr* storageKeyItem = (*dckArrayItem)[0]->asBstr();
    const cppbor::Bstr* credentialPrivKeyItem = (*dckArrayItem)[1]->asBstr();
    if (storageKeyItem == nullptr || credentialPrivKeyItem == nullptr) {
        LOG(ERROR) << "CredentialKeys unexpected item types";
        return ResultCode::INVALID_DATA;
    }
    storageKey_ = storageKeyItem->value();
    credentialPrivKey_ = credentialPrivKeyItem->value();

    return ResultCode::OK;
}

Return<void> IdentityCredential::startRetrieval(
        const hidl_vec<SecureAccessControlProfile>& accessControlProfiles,
        const hidl_vec<uint8_t>& authToken, const hidl_vec<uint8_t>& itemsRequest,
        const hidl_vec<uint8_t>& sessionTranscript, const hidl_vec<uint8_t>& readerSignature,
        const hidl_vec<uint16_t>& requestCounts, startRetrieval_cb _hidl_cb) {
    if (sessionTranscript.size() > 0) {
        auto [item, _, message] = cppbor::parse(sessionTranscript);
        if (item == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "SessionTranscript contains invalid CBOR"));
            return Void();
        }
        sessionTranscriptItem_ = std::move(item);
    }
    sessionTranscript_ = sessionTranscript;

    // If there is a signature, validate that it was made with the top-most key in the
    // certificate chain embedded in the COSE_Sign1 structure.
    vector<uint8_t> readerCertificateChain;
    if (readerSignature.size() > 0) {
        if (!support::coseSignGetX5Chain(readerSignature, readerCertificateChain)) {
            _hidl_cb(support::result(ResultCode::READER_SIGNATURE_CHECK_FAILED,
                                     "Unable to get reader certificate chain from COSE_Sign1"));
            return Void();
        }

        if (!support::certificateChainValidate(readerCertificateChain)) {
            _hidl_cb(support::result(ResultCode::READER_SIGNATURE_CHECK_FAILED,
                                     "Error validating reader certificate chain"));
            return Void();
        }

        vector<uint8_t> readerPublicKey;
        if (!support::certificateChainGetTopMostKey(readerCertificateChain, readerPublicKey)) {
            _hidl_cb(support::result(ResultCode::READER_SIGNATURE_CHECK_FAILED,
                                     "Unable to get public key from reader certificate chain"));
            return Void();
        }

        const vector<uint8_t>& itemsRequestBytes = itemsRequest;
        vector<uint8_t> dataThatWasSigned = cppbor::Array()
                                                    .add("ReaderAuthentication")
                                                    .add(sessionTranscriptItem_->clone())
                                                    .add(cppbor::Semantic(24, itemsRequestBytes))
                                                    .encode();
        if (!support::coseCheckEcDsaSignature(readerSignature,
                                              dataThatWasSigned,  // detached content
                                              readerPublicKey)) {
            _hidl_cb(support::result(ResultCode::READER_SIGNATURE_CHECK_FAILED,
                                     "readerSignature check failed"));
            return Void();
        }
    }

    // Validate the passed-in |authToken|
    if (!support::validateAuthToken(authToken)) {
        _hidl_cb(support::result(ResultCode::INVALID_AUTH_TOKEN, "authToken validation failed"));
        return Void();
    }

    // To prevent replay-attacks, we check that the public part of the ephemeral
    // key we previously created, is present in the DeviceEngagement part of
    // SessionTranscript as a COSE_Key, in uncompressed form.
    //
    // We do this by just searching for the X and Y coordinates.
    if (sessionTranscript.size() > 0) {
        const cppbor::Array* array = sessionTranscriptItem_->asArray();
        if (array == nullptr || array->size() != 2) {
            _hidl_cb(support::result(ResultCode::EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                                     "SessionTranscript is not an array with two items"));
            return Void();
        }
        const cppbor::Semantic* taggedEncodedDE = (*array)[0]->asSemantic();
        if (taggedEncodedDE == nullptr || taggedEncodedDE->value() != 24) {
            _hidl_cb(support::result(ResultCode::EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                                     "First item in SessionTranscript array is not a "
                                     "semantic with value 24"));
            return Void();
        }
        const cppbor::Bstr* encodedDE = (taggedEncodedDE->child())->asBstr();
        if (encodedDE == nullptr) {
            _hidl_cb(support::result(ResultCode::EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                                     "Child of semantic in first item in SessionTranscript "
                                     "array is not a bstr"));
            return Void();
        }
        const vector<uint8_t>& bytesDE = encodedDE->value();

        vector<uint8_t> ePubX;
        vector<uint8_t> ePubY;
        if (!support::ecPublicKeyGetXandY(ephemeralPublicKey_, ePubX, ePubY)) {
            _hidl_cb(support::result(ResultCode::EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                                     "Error extracting X and Y from ePub"));
            return Void();
        }
        if (sessionTranscript.size() > 0 &&
            !(support::cborHasBstr(bytesDE, ePubX) && support::cborHasBstr(bytesDE, ePubY))) {
            _hidl_cb(support::result(ResultCode::EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
                                     "Did not find ephemeral public key's X and Y coordinates in "
                                     "SessionTranscript (make sure leading zeroes are not used)"));
            return Void();
        }
    }

    // itemsRequest: If non-empty, contains request data that may be signed by the
    // reader.  The content can be defined in the way appropriate for the
    // credential, but there are three requirements that must be met to work with
    // this HAL:
    if (itemsRequest.size() > 0) {
        // 1. The content must be a CBOR-encoded structure.
        auto [item, _, message] = cppbor::parse(itemsRequest);
        if (item == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_ITEMS_REQUEST_MESSAGE,
                                     "Error decoding CBOR in itemsRequest: %s", message.c_str()));
            return Void();
        }

        // 2. The CBOR structure must be a map.
        const cppbor::Map* map = item->asMap();
        if (map == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_ITEMS_REQUEST_MESSAGE,
                                     "itemsRequest is not a CBOR map"));
            return Void();
        }

        // 3. The map must contain a key "nameSpaces" whose value contains a map, as described in
        //    the example below.
        //
        //   NameSpaces = {
        //     + NameSpace => DataElements ; Requested data elements for each NameSpace
        //   }
        //
        //   NameSpace = tstr
        //
        //   DataElements = {
        //     + DataElement => IntentToRetain
        //   }
        //
        //   DataElement = tstr
        //   IntentToRetain = bool
        //
        // Here's an example of an |itemsRequest| CBOR value satisfying above requirements 1.
        // through 3.:
        //
        //    {
        //        'docType' : 'org.iso.18013-5.2019',
        //        'nameSpaces' : {
        //            'org.iso.18013-5.2019' : {
        //                'Last name' : false,
        //                'Birth date' : false,
        //                'First name' : false,
        //                'Home address' : true
        //            },
        //            'org.aamva.iso.18013-5.2019' : {
        //                'Real Id' : false
        //            }
        //        }
        //    }
        //
        const cppbor::Map* nsMap = nullptr;
        for (size_t n = 0; n < map->size(); n++) {
            const auto& [keyItem, valueItem] = (*map)[n];
            if (keyItem->type() == cppbor::TSTR && keyItem->asTstr()->value() == "nameSpaces" &&
                valueItem->type() == cppbor::MAP) {
                nsMap = valueItem->asMap();
                break;
            }
        }
        if (nsMap == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_ITEMS_REQUEST_MESSAGE,
                                     "No nameSpaces map in top-most map"));
            return Void();
        }

        for (size_t n = 0; n < nsMap->size(); n++) {
            auto [nsKeyItem, nsValueItem] = (*nsMap)[n];
            const cppbor::Tstr* nsKey = nsKeyItem->asTstr();
            const cppbor::Map* nsInnerMap = nsValueItem->asMap();
            if (nsKey == nullptr || nsInnerMap == nullptr) {
                _hidl_cb(support::result(ResultCode::INVALID_ITEMS_REQUEST_MESSAGE,
                                         "Type mismatch in nameSpaces map"));
                return Void();
            }
            string requestedNamespace = nsKey->value();
            vector<string> requestedKeys;
            for (size_t m = 0; m < nsInnerMap->size(); m++) {
                const auto& [innerMapKeyItem, innerMapValueItem] = (*nsInnerMap)[m];
                const cppbor::Tstr* nameItem = innerMapKeyItem->asTstr();
                const cppbor::Simple* simple = innerMapValueItem->asSimple();
                const cppbor::Bool* intentToRetainItem =
                        (simple != nullptr) ? simple->asBool() : nullptr;
                if (nameItem == nullptr || intentToRetainItem == nullptr) {
                    _hidl_cb(support::result(ResultCode::INVALID_ITEMS_REQUEST_MESSAGE,
                                             "Type mismatch in value in nameSpaces map"));
                    return Void();
                }
                requestedKeys.push_back(nameItem->value());
            }
            requestedNameSpacesAndNames_[requestedNamespace] = requestedKeys;
        }
    }

    // Finally, validate all the access control profiles in the requestData.
    for (const auto& profile : accessControlProfiles) {
        if (!support::secureAccessControlProfileCheckMac(profile, storageKey_)) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Error checking MAC for profile with id %d", int(profile.id)));
            return Void();
        }
        ResultCode accessControlCheck = ResultCode::OK;
        if (!support::checkUserAuthentication(profile, authToken)) {
            accessControlCheck = ResultCode::USER_AUTHENTICATION_FAILED;
        } else if (profile.readerCertificate.size() > 0) {
            if (!support::checkReaderAuthentication(profile, readerCertificateChain)) {
                accessControlCheck = ResultCode::READER_AUTHENTICATION_FAILED;
            }
        }
        profileIdToAccessCheckResult_[profile.id] = accessControlCheck;
    }

    deviceNameSpacesMap_ = cppbor::Map();
    currentNameSpaceDeviceNameSpacesMap_ = cppbor::Map();

    requestCountsRemaining_ = requestCounts;
    currentNameSpace_ = "";

    itemsRequest_ = itemsRequest;

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> IdentityCredential::startRetrieveEntryValue(
        const hidl_string& nameSpace, const hidl_string& name, uint32_t entrySize,
        const hidl_vec<uint16_t>& accessControlProfileIds, bool authenticateEntry,
        startRetrieveEntryValue_cb _hidl_cb) {
    if (name.empty()) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Name cannot be empty"));
        return Void();
    }
    if (nameSpace.empty()) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Name space cannot be empty"));
        return Void();
    }

    if (requestCountsRemaining_.size() == 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "No more name spaces left to go through"));
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
        if (currentNameSpaceDeviceNameSpacesMap_.size() > 0) {
            deviceNameSpacesMap_.add(currentNameSpace_,
                                     std::move(currentNameSpaceDeviceNameSpacesMap_));
        }
        currentNameSpaceDeviceNameSpacesMap_ = cppbor::Map();

        requestCountsRemaining_.erase(requestCountsRemaining_.begin());
        currentNameSpace_ = nameSpace;
    }

    // It's permissible to have an empty itemsRequest... but if non-empty you can
    // only request what was specified in said itemsRequest. Enforce that.
    if (itemsRequest_.size() > 0) {
        const auto& it = requestedNameSpacesAndNames_.find(nameSpace);
        if (it == requestedNameSpacesAndNames_.end()) {
            _hidl_cb(support::result(ResultCode::NOT_IN_REQUEST_MESSAGE,
                                     "Name space '%s' was not requested in startRetrieval",
                                     nameSpace.c_str()));
            return Void();
        }
        const auto& dataItemNames = it->second;
        if (std::find(dataItemNames.begin(), dataItemNames.end(), name) == dataItemNames.end()) {
            _hidl_cb(support::result(
                    ResultCode::NOT_IN_REQUEST_MESSAGE,
                    "Data item name '%s' in name space '%s' was not requested in startRetrieval",
                    name.c_str(), nameSpace.c_str()));
            return Void();
        }
    }

    // Enforce access control.
    //
    // Access is granted if at least one of the profiles grants access.
    //
    // If an item is configured without any profiles, access is denied.
    //
    ResultCode accessControl = ResultCode::NO_ACCESS_CONTROL_PROFILES;
    for (auto id : accessControlProfileIds) {
        auto search = profileIdToAccessCheckResult_.find(id);
        if (search == profileIdToAccessCheckResult_.end()) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Requested entry with unvalidated profile id %d", (int(id))));
            return Void();
        }
        ResultCode accessControlForProfile = search->second;
        if (accessControlForProfile == ResultCode::OK) {
            accessControl = ResultCode::OK;
            break;
        }
        accessControl = accessControlForProfile;
    }
    if (accessControl != ResultCode::OK) {
        _hidl_cb(support::result(accessControl, "Access control check failed"));
        return Void();
    }

    if (!support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
                                            entryAdditionalData_)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating AdditionalData"));
        return Void();
    }

    currentName_ = name;
    entryRemainingBytes_ = entrySize;
    entryValue_.resize(0);
    entryAuthenticate_ = authenticateEntry;

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> IdentityCredential::retrieveEntryValue(const hidl_vec<uint8_t>& encryptedContent,
                                                    retrieveEntryValue_cb _hidl_cb) {
    vector<uint8_t> content;
    if (!support::decryptAes128Gcm(storageKey_, encryptedContent, entryAdditionalData_, content)) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error decrypting data"), {});
        return Void();
    }

    size_t chunkSize = content.size();

    if (chunkSize > entryRemainingBytes_) {
        LOG(ERROR) << "Retrieved chunk of size " << chunkSize
                   << " is bigger than remaining space of size " << entryRemainingBytes_;
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "Retrieved chunk of size %zd is bigger than remaining space "
                                 "of size %zd",
                                 chunkSize, entryRemainingBytes_),
                 {});
        return Void();
    }

    entryRemainingBytes_ -= chunkSize;
    if (entryRemainingBytes_ > 0) {
        if (chunkSize != IdentityCredentialStore::kGcmChunkSize) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Retrieved non-final chunk of size %zd but expected "
                                     "kGcmChunkSize which is %zd",
                                     chunkSize, IdentityCredentialStore::kGcmChunkSize),
                     {});
            return Void();
        }
    }

    entryValue_.insert(entryValue_.end(), content.begin(), content.end());

    if (entryRemainingBytes_ == 0 && entryAuthenticate_) {
        auto [entryValueItem, _, message] = cppbor::parse(entryValue_);
        if (entryValueItem == nullptr) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Retrieved data invalid CBOR"), {});
            return Void();
        }
        currentNameSpaceDeviceNameSpacesMap_.add(currentName_, std::move(entryValueItem));
    }

    _hidl_cb(support::resultOK(), content);
    return Void();
}

Return<void> IdentityCredential::finishRetrieval(
        const hidl_vec<uint8_t>& signingKeyBlob,
        const hidl_vec<uint8_t>& previousAuditSignatureHash, finishRetrieval_cb _hidl_cb) {
    if (currentNameSpaceDeviceNameSpacesMap_.size() > 0) {
        deviceNameSpacesMap_.add(currentNameSpace_,
                                 std::move(currentNameSpaceDeviceNameSpacesMap_));
    }
    vector<uint8_t> encodedDeviceNameSpaces = deviceNameSpacesMap_.encode();

    // If there's no signing key or no sessionTranscript, we return the empty signature.
    vector<uint8_t> signature;
    if (signingKeyBlob.size() > 0 && sessionTranscript_.size() > 0) {
        cppbor::Array array;
        array.add("DeviceAuthentication");
        array.add(sessionTranscriptItem_->clone());
        array.add(docType_);
        array.add(cppbor::Semantic(24, encodedDeviceNameSpaces));
        vector<uint8_t> encodedDeviceAuthentication = array.encode();
        vector<uint8_t> signingKey;
        vector<uint8_t> docTypeAsBlob(docType_.begin(), docType_.end());
        if (!support::decryptAes128Gcm(storageKey_, signingKeyBlob, docTypeAsBlob, signingKey)) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA, "Error decrypting signingKeyBlob"),
                     {}, {}, {});
            return Void();
        }

        if (!support::coseSignEcDsa(signingKey, {},               // content,
                                    encodedDeviceAuthentication,  // detached content
                                    {},                           // certificate chain to include
                                    signature)) {
            _hidl_cb(support::result(ResultCode::FAILED, "Error signing DeviceAuthentication"), {},
                     {}, {});
            return Void();
        }
    }

    // TODO: We need to rethink the whole presentation log since both requests and responses
    //       can be filtered. Also want to tie it in with what binder API the logviewer component
    //       is going to need.
    //
    /*
    if (previousAuditSignatureHash.size() != 32) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "Unexpected previousAuditSignatureHash size"),
                 {}, {}, {});
        return Void();
    }
    vector<uint8_t> requestHash = support::sha256(itemsRequest_);
    vector<uint8_t> responseHash = support::sha256(encodedDeviceNameSpaces);
    cppbor::Array auditArray;
    auditArray.add("AuditLogEntry");
    auditArray.add(cppbor::Bstr(requestHash));
    auditArray.add(cppbor::Bstr(responseHash));
    auditArray.add(cppbor::Bstr(previousAuditSignatureHash));
    vector<uint8_t> encodedAuditLogData = auditArray.encode();
    vector<uint8_t> auditLogSignature;
    if (!support::signEcDsa(credentialPrivKey_, encodedAuditLogData, auditLogSignature)) {
      _hidl_cb(support::result(ResultCode::FAILED, "Error signing AuditLogData"), {}, {}, {});
        return Void();
    }
    memcpy(auditLogEntry.requestHash.data(), requestHash.data(), 32);
    memcpy(auditLogEntry.responseHash.data(), responseHash.data(), 32);
    auditLogEntry.signature = auditLogSignature;
    */
    AuditLogEntry auditLogEntry;

    _hidl_cb(support::resultOK(), signature, encodedDeviceNameSpaces, auditLogEntry);
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

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity
}  // namespace hardware
}  // namespace android
