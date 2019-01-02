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

#include "WritableIdentityCredential.h"
#include "IdentityCredentialStore.h"

#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>
#include <android/hardware/keymaster/capability/1.0/types.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using std::vector;

// Writes CBOR-encoded structure to |credentialKeys| containing |storageKey| and
// |credentialPrivKey|.
static bool generateCredentialKeys(const vector<uint8_t>& storageKey,
                                   const vector<uint8_t>& credentialPrivKey,
                                   vector<uint8_t>& credentialKeys) {
    if (storageKey.size() != 16) {
        LOG(ERROR) << "Size of storageKey is not 16";
        return false;
    }

    cn_cbor_errback err;
    auto array = support::CnCborPtr(cn_cbor_array_create(&err));
    if (array.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating array (pos " << err.pos << ")";
        return false;
    }

    cn_cbor* storageKeyBStr = cn_cbor_data_create(storageKey.data(), storageKey.size(), &err);
    if (storageKeyBStr == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    if (!cn_cbor_array_append(array.get(), storageKeyBStr, &err)) {
        LOG(ERROR) << "Error " << err.err << " appending to array (pos " << err.pos << ")";
        cn_cbor_free(storageKeyBStr);
        return false;
    }

    cn_cbor* credentialPrivKeyBStr =
        cn_cbor_data_create(credentialPrivKey.data(), credentialPrivKey.size(), &err);
    if (credentialPrivKeyBStr == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    if (!cn_cbor_array_append(array.get(), credentialPrivKeyBStr, &err)) {
        LOG(ERROR) << "Error " << err.err << " appending to array (pos " << err.pos << ")";
        cn_cbor_free(credentialPrivKeyBStr);
        return false;
    }

    if (!support::cborEncode(array.get(), credentialKeys)) {
        return false;
    }

    return true;
}

Return<void> WritableIdentityCredential::getAttestationCertificate(
    const hidl_vec<uint8_t>& attestationApplicationId,
    const hidl_vec<uint8_t>& attestationChallenge, getAttestationCertificate_cb _hidl_cb) {
    if (storageKey_.size() > 0) {
        LOG(ERROR) << "getAttestationCertificate() already called";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    vector<uint8_t> certificate;
    if (!support::createEcKeyAndAttestationChain(credentialPrivKey_, certificate)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    // Initialize storageKey_ which will be used in multiple hwbinder calls.
    if (!support::getRandom(16, storageKey_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey_, credentialKeys)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    vector<uint8_t> hardwareBoundKey;
    if (testCredential_) {
        hardwareBoundKey = support::getTestHardwareBoundKey();
    } else {
        hardwareBoundKey = support::getHardwareBoundKey();
    }

    // Calculate |credentialData_| which is used in finishAddingEntries().
    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    vector<uint8_t> credentialBlob;
    vector<uint8_t> docTypeVec(docType_.begin(), docType_.end());
    if (!support::encryptAes128Gcm(hardwareBoundKey, nonce, credentialKeys, docTypeVec,
                                   credentialBlob)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    cn_cbor_errback err;
    auto array = support::CnCborPtr(cn_cbor_array_create(&err));
    if (array.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating array (pos " << err.pos << ")";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!support::cborArrayAppendString(array.get(), docType_.c_str())) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!support::cborArrayAppendBool(array.get(), testCredential_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!support::cborArrayAppendBStr(array.get(), credentialBlob.data(), credentialBlob.size())) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    if (!support::cborEncode(array.get(), credentialData_)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    _hidl_cb(ResultCode::OK, certificate);
    return Void();
}

Return<ResultCode> WritableIdentityCredential::startPersonalization(
    uint8_t accessControlProfileCount, const hidl_vec<uint16_t>& entryCounts) {
    if (storageKey_.size() == 0) {
        LOG(ERROR) << "getAttestationCertificate() not yet called";
        return ResultCode::FAILED;
    }

    numAccessControlProfileRemaining_ = accessControlProfileCount;
    remainingEntryCounts_ = entryCounts;
    entryNameSpace_ = "";
    accessControlProfiles_.resize(0);
    signedDataBuilder_.reset(docType_, testCredential_);
    return ResultCode::OK;
}

static bool secureAccessControlProfileCalcMac(const SecureAccessControlProfile& profile,
                                              const vector<uint8_t>& storageKey,
                                              vector<uint8_t>& mac) {
    cn_cbor_errback err;

    auto map = support::CnCborPtr(cn_cbor_map_create(&err));
    if (map.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return false;
    }
    if (!support::cborMapPutStringInt(map.get(), "id", profile.id)) {
        return false;
    }

    if (profile.readerAuthPubKey.size() > 0) {
        if (!support::cborMapPutStringBStr(map.get(), "readerAuthPubKey",
                                           profile.readerAuthPubKey.data(),
                                           profile.readerAuthPubKey.size())) {
            return false;
        }
    }

    if (profile.capabilityId != 0) {
        if (!support::cborMapPutStringInt(map.get(), "capabilityId", profile.capabilityId)) {
            return false;
        }
    }
    if (profile.capabilityType !=
        ::android::hardware::keymaster::capability::V1_0::CapabilityType::NOT_APPLICABLE) {
        if (!support::cborMapPutStringInt(map.get(), "capabilityType",
                                          int64_t(profile.capabilityType))) {
            return false;
        }
    }

    vector<uint8_t> cborData;
    if (!support::cborEncode(map.get(), cborData)) {
        return false;
    }
    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        return false;
    }
    if (!support::encryptAes128Gcm(storageKey, nonce, {}, cborData, mac)) {
        return false;
    }

    return true;
}

Return<void> WritableIdentityCredential::addAccessControlProfile(
    uint8_t id, const hidl_vec<uint8_t>& readerAuthPubKey, uint64_t capabilityId,
    CapabilityType capabilityType, uint32_t timeout, addAccessControlProfile_cb _hidl_cb) {
    SecureAccessControlProfile profile;

    if (numAccessControlProfileRemaining_ == 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is 0 and expected non-zero";
        _hidl_cb(ResultCode::FAILED, profile);
        return Void();
    }

    // Spec requires if |capabilityId| is zero, then |timeout| must also be zero.
    if (capabilityId == 0 && timeout != 0) {
        LOG(ERROR) << "capabilityId is 0 but timeout is non-zero";
        _hidl_cb(ResultCode::FAILED, profile);
        return Void();
    }

    profile.id = id;
    profile.readerAuthPubKey = readerAuthPubKey;
    profile.capabilityId = capabilityId;
    profile.capabilityType = capabilityType;
    profile.timeout = timeout;
    vector<uint8_t> mac;
    if (!secureAccessControlProfileCalcMac(profile, storageKey_, mac)) {
        _hidl_cb(ResultCode::FAILED, profile);
        return Void();
    }
    profile.mac = mac;
    accessControlProfiles_.push_back(profile);

    numAccessControlProfileRemaining_--;

    signedDataBuilder_.addAccessControlProfile(profile);

    _hidl_cb(ResultCode::OK, profile);
    return Void();
}

Return<ResultCode> WritableIdentityCredential::beginAddEntry(
    const hidl_vec<uint8_t>& accessControlProfileIds, const hidl_string& nameSpace,
    const hidl_string& name, bool directlyAvailable, uint32_t entrySize) {
    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        return ResultCode::FAILED;
    }

    if (remainingEntryCounts_.size() == 0) {
        LOG(ERROR) << "No more namespaces to add to";
        return ResultCode::FAILED;
    }

    // Handle initial beginEntry() call.
    if (entryNameSpace_ == "") {
        entryNameSpace_ = nameSpace;
    }

    // If the namespace changed...
    if (nameSpace != entryNameSpace_) {
        // Then check that all entries in the previous namespace have been added..
        if (remainingEntryCounts_[0] != 0) {
            LOG(ERROR) << "New namespace but " << remainingEntryCounts_[0]
                       << " entries remain to be added";
            return ResultCode::FAILED;
        }
        remainingEntryCounts_.erase(remainingEntryCounts_.begin());
    } else {
        // Same namespace...
        if (remainingEntryCounts_[0] == 0) {
            LOG(ERROR) << "Same namespace but no entries remain to be added";
            return ResultCode::FAILED;
        }
        remainingEntryCounts_[0] -= 1;
    }

    // TODO: validate |accessControlProfiles|

    if (!support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
                                            entryAdditionalData_)) {
        return ResultCode::FAILED;
    }

    entryRemainingBytes_ = entrySize;
    entryNameSpace_ = nameSpace;
    entryName_ = name;
    entryAccessControlProfileIds_ = accessControlProfileIds;
    entryDirectlyAvailable_ = directlyAvailable;
    entryBStrValue_.resize(0);
    entryStrValue_.resize(0);

    return ResultCode::OK;
}

Return<void> WritableIdentityCredential::addEntryValue(const EntryValue& value,
                                                       addEntryValue_cb _hidl_cb) {
    cn_cbor* cbor_value;
    cn_cbor_errback err;
    ssize_t chunkSize = -1;
    switch (value.getDiscriminator()) {
        case EntryValue::hidl_discriminator::integer:
            cbor_value = cn_cbor_int_create(value.integer(), &err);
            break;
        case EntryValue::hidl_discriminator::textString:
            chunkSize = value.textString().size();
            cbor_value = cn_cbor_string_create(value.textString().c_str(), &err);
            entryStrValue_.append(value.textString());
            break;
        case EntryValue::hidl_discriminator::byteString:
            chunkSize = value.byteString().size();
            cbor_value =
                cn_cbor_data_create(value.byteString().data(), value.byteString().size(), &err);
            entryBStrValue_.insert(entryBStrValue_.end(), value.byteString().begin(),
                                   value.byteString().end());
            break;
        case EntryValue::hidl_discriminator::booleanValue:
            // There is no cn_cbor_bool_create()
            cbor_value = (cn_cbor*)calloc(1, sizeof(cn_cbor));
            cbor_value->type = value.booleanValue() ? CN_CBOR_TRUE : CN_CBOR_TRUE;
            break;
    }
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating object (pos " << err.pos << ")";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    if (chunkSize < 0) {
        // This is the case where the value is not a tstr/bstr... make sure that
        // that 0 was passed as |entrySize| in beginAddEntry().
        if (entryRemainingBytes_ != 0) {
            LOG(ERROR) << "Passed in non-zero entrySize " << entryRemainingBytes_
                       << " to beginAddEntry() but value passed to addEntryValue() is a tstr/bstr";
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
    } else {
        // value is a tstr/bstr
        if (size_t(chunkSize) > IdentityCredentialStore::kGcmChunkSize) {
            LOG(ERROR) << "Passed in chunk of size " << chunkSize
                       << " is bigger than kGcmChunkSize which is "
                       << IdentityCredentialStore::kGcmChunkSize;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
        if (size_t(chunkSize) > entryRemainingBytes_) {
            LOG(ERROR) << "Passed in chunk of size " << chunkSize
                       << " is bigger than remaining space of size " << entryRemainingBytes_;
            _hidl_cb(ResultCode::FAILED, {});
            return Void();
        }
        entryRemainingBytes_ -= chunkSize;
        if (entryRemainingBytes_ > 0) {
            if (size_t(chunkSize) != IdentityCredentialStore::kGcmChunkSize) {
                LOG(ERROR) << "Retrieved non-final chunk of size " << chunkSize
                           << " but expected kGcmChunkSize which is"
                           << IdentityCredentialStore::kGcmChunkSize;
                _hidl_cb(ResultCode::FAILED, {});
                return Void();
            }
        }
    }

    vector<uint8_t> cborData;
    if (!support::cborEncode(cbor_value, cborData)) {
        cn_cbor_free(cbor_value);
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    cn_cbor_free(cbor_value);

    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    vector<uint8_t> encryptedContent;
    if (!support::encryptAes128Gcm(storageKey_, nonce, cborData, entryAdditionalData_,
                                   encryptedContent)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
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
        signedDataBuilder_.addEntry(entryNameSpace_, entryName_, entryAccessControlProfileIds_,
                                    valueToWrite, entryDirectlyAvailable_);
    }

    _hidl_cb(ResultCode::OK, encryptedContent);
    return Void();
}

Return<void> WritableIdentityCredential::finishAddingEntries(finishAddingEntries_cb _hidl_cb) {
    vector<uint8_t> signature;

    vector<uint8_t> encodedCbor;
    if (!signedDataBuilder_.getEncodedCbor(encodedCbor)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    if (!support::signEcDsa(credentialPrivKey_, encodedCbor, signature)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    _hidl_cb(ResultCode::OK, credentialData_, signature);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
