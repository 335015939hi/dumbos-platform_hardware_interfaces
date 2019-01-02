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

using std::string;
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

// Writes CBOR-encoded structure to |credentialData| containing |docType|,
// |testCredential| and |credentialKeys|. The latter element will be stored in
// encrypted form, using |hardwareBoundKey| as the encryption key.
bool generateCredentialData(const vector<uint8_t>& hardwareBoundKey, const string& docType,
                            bool testCredential, const vector<uint8_t>& credentialKeys,
                            vector<uint8_t>& credentialData) {
    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        LOG(ERROR) << "Error getting random";
        return false;
    }
    vector<uint8_t> credentialBlob;
    vector<uint8_t> docTypeAsVec(docType.begin(), docType.end());
    if (!support::encryptAes128Gcm(hardwareBoundKey, nonce, credentialKeys, docTypeAsVec,
                                   credentialBlob)) {
        LOG(ERROR) << "Error encrypting CredentialKeys blob";
        return false;
    }

    cn_cbor_errback err;
    auto array = support::CnCborPtr(cn_cbor_array_create(&err));
    if (array.get() == nullptr || !support::cborArrayAppendString(array.get(), docType.c_str()) ||
        !support::cborArrayAppendBool(array.get(), testCredential) ||
        !support::cborArrayAppendBStr(array.get(), credentialBlob.data(), credentialBlob.size()) ||
        !support::cborEncode(array.get(), credentialData)) {
        LOG(ERROR) << "Error creating CredentialData array";
        return false;
    }
    return true;
}

bool WritableIdentityCredential::initialize() {
    vector<uint8_t> keyPair;
    if (!support::createEcKeyPair(keyPair) ||
        !support::ecKeyPairGetPublicKey(keyPair, credentialPubKey_) ||
        !support::ecKeyPairGetPrivateKey(keyPair, credentialPrivKey_)) {
        LOG(ERROR) << "Error creating credentialKey";
        return false;
    }

    if (!support::getRandom(16, storageKey_)) {
        LOG(ERROR) << "Error creating storageKey";
        return false;
    }

    return true;
}

Return<void> WritableIdentityCredential::getAttestationCertificate(
    const hidl_vec<uint8_t>& attestationChallenge, getAttestationCertificate_cb _hidl_cb) {
    // For now, we dynamically generate an attestion key on each and every
    // request and use that to sign CredentialKey. In a real implementation this
    // would look very differently.
    vector<uint8_t> attestationKeyPair;
    vector<uint8_t> attestationPrivKey;
    vector<uint8_t> attestationPubKey;
    if (!support::createEcKeyPair(attestationKeyPair) ||
        !support::ecKeyPairGetPublicKey(attestationKeyPair, attestationPubKey) ||
        !support::ecKeyPairGetPrivateKey(attestationKeyPair, attestationPrivKey)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating attestationKey"), {});
        return Void();
    }

    string serialDecimal;
    string issuer;
    string subject;
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    // First create a certificate for |credentialPubKey| which is signed by
    // |attestationPrivKey|.
    //
    serialDecimal = "0";  // TODO: set serial to |attestationChallenge|
    issuer = "Android Open Source Project";
    subject = "Android IdentityCredential CredentialKey";
    vector<uint8_t> credentialPubKeyCertificate;
    if (!support::ecPublicKeyGenerateCertificate(credentialPubKey_, attestationPrivKey,
                                                 serialDecimal, issuer, subject, validityNotBefore,
                                                 validityNotAfter, credentialPubKeyCertificate)) {
        _hidl_cb(
            support::result(ResultCode::FAILED, "Error creating certificate for credentialPubKey"),
            {});
        return Void();
    }

    // This is follewed by a certificate for |attestationPubKey| self-signed by
    // |attestationPrivKey|.
    serialDecimal = "0";  // TODO: set serial
    issuer = "Android Open Source Project";
    subject = "Android IdentityCredential AttestationKey";
    vector<uint8_t> attestationKeyCertificate;
    if (!support::ecPublicKeyGenerateCertificate(attestationPubKey, attestationPrivKey,
                                                 serialDecimal, issuer, subject, validityNotBefore,
                                                 validityNotAfter, attestationKeyCertificate)) {
        _hidl_cb(
            support::result(ResultCode::FAILED, "Error creating certificate for attestationPubKey"),
            {});
        return Void();
    }

    // Concatenate the certificates to form the chain.
    vector<uint8_t> certificateChain;
    certificateChain.insert(certificateChain.end(), credentialPubKeyCertificate.begin(),
                            credentialPubKeyCertificate.end());
    certificateChain.insert(certificateChain.end(), attestationKeyCertificate.begin(),
                            attestationKeyCertificate.end());

    _hidl_cb(support::resultOK(), certificateChain);
    return Void();
}

Return<void> WritableIdentityCredential::startPersonalization(uint8_t accessControlProfileCount,
                                                              const hidl_vec<uint16_t>& entryCounts,
                                                              startPersonalization_cb _hidl_cb) {
    numAccessControlProfileRemaining_ = accessControlProfileCount;
    remainingEntryCounts_ = entryCounts;
    entryNameSpace_ = "";
    accessControlProfiles_.resize(0);
    signedDataBuilder_.reset(docType_, testCredential_);
    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> WritableIdentityCredential::addAccessControlProfile(
    uint8_t id, const hidl_vec<uint8_t>& readerCertificate, uint64_t capabilityId,
    CapabilityType capabilityType, uint32_t timeout, addAccessControlProfile_cb _hidl_cb) {
    SecureAccessControlProfile profile;

    if (numAccessControlProfileRemaining_ == 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "numAccessControlProfileRemaining_ is 0 and expected non-zero"),
                 profile);
        return Void();
    }

    // Spec requires if |capabilityId| is zero, then |timeout| must also be zero.
    if (capabilityId == 0 && timeout != 0) {
        _hidl_cb(
            support::result(ResultCode::INVALID_DATA, "capabilityId is 0 but timeout is non-zero"),
            profile);
        return Void();
    }

    profile.id = id;
    profile.readerCertificate = readerCertificate;
    profile.capabilityId = capabilityId;
    profile.capabilityType = capabilityType;
    profile.timeout = timeout;
    vector<uint8_t> mac;
    if (!support::secureAccessControlProfileCalcMac(profile, storageKey_, mac)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error calculating MAC for profile"), profile);
        return Void();
    }
    profile.mac = mac;
    accessControlProfiles_.push_back(profile);

    numAccessControlProfileRemaining_--;

    signedDataBuilder_.addAccessControlProfile(profile);

    _hidl_cb(support::resultOK(), profile);
    return Void();
}

Return<void> WritableIdentityCredential::beginAddEntry(
    const hidl_vec<uint8_t>& accessControlProfileIds, const hidl_string& nameSpace,
    const hidl_string& name, bool directlyAvailable, uint32_t entrySize,
    beginAddEntry_cb _hidl_cb) {
    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                 "numAccessControlProfileRemaining_ is %zd and expected zero",
                                 numAccessControlProfileRemaining_));
        return Void();
    }

    if (remainingEntryCounts_.size() == 0) {
        _hidl_cb(support::result(ResultCode::INVALID_DATA, "No more namespaces to add to"));
        return Void();
    }

    // Handle initial beginEntry() call.
    if (entryNameSpace_ == "") {
        entryNameSpace_ = nameSpace;
    }

    // If the namespace changed...
    if (nameSpace != entryNameSpace_) {
        // Then check that all entries in the previous namespace have been added..
        if (remainingEntryCounts_[0] != 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "New namespace but %d entries remain to be added",
                                     int(remainingEntryCounts_[0])));
            return Void();
        }
        remainingEntryCounts_.erase(remainingEntryCounts_.begin());
    } else {
        // Same namespace...
        if (remainingEntryCounts_[0] == 0) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Same namespace but no entries remain to be added"));
            return Void();
        }
        remainingEntryCounts_[0] -= 1;
    }

    if (!support::entryCreateAdditionalData(nameSpace, name, accessControlProfileIds,
                                            entryAdditionalData_)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error creating AdditionalData for entry"));
        return Void();
    }

    entryRemainingBytes_ = entrySize;
    entryNameSpace_ = nameSpace;
    entryName_ = name;
    entryAccessControlProfileIds_ = accessControlProfileIds;
    entryDirectlyAvailable_ = directlyAvailable;
    entryBStrValue_.resize(0);
    entryStrValue_.resize(0);

    _hidl_cb(support::resultOK());
    return Void();
}

Return<void> WritableIdentityCredential::addEntryValue(const EntryValue& value,
                                                       addEntryValue_cb _hidl_cb) {
    cn_cbor* cborValue;
    cn_cbor_errback err;
    ssize_t chunkSize = -1;
    switch (value.getDiscriminator()) {
        case EntryValue::hidl_discriminator::integer:
            cborValue = cn_cbor_int_create(value.integer(), &err);
            break;
        case EntryValue::hidl_discriminator::textString:
            chunkSize = value.textString().size();
            cborValue =
                cn_cbor_data_create(value.textString().data(), value.textString().size(), &err);
            cborValue->type = CN_CBOR_TEXT;
            entryStrValue_.insert(entryStrValue_.end(), value.textString().begin(),
                                  value.textString().end());
            break;
        case EntryValue::hidl_discriminator::byteString:
            chunkSize = value.byteString().size();
            cborValue =
                cn_cbor_data_create(value.byteString().data(), value.byteString().size(), &err);
            entryBStrValue_.insert(entryBStrValue_.end(), value.byteString().begin(),
                                   value.byteString().end());
            break;
        case EntryValue::hidl_discriminator::booleanValue:
            // There is no cn_cbor_bool_create()
            cborValue = (cn_cbor*)calloc(1, sizeof(cn_cbor));
            cborValue->type = value.booleanValue() ? CN_CBOR_TRUE : CN_CBOR_TRUE;
            break;
    }
    if (cborValue == nullptr) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error %d creating CBOR object", int(err.err)),
                 {});
        return Void();
    }

    if (chunkSize < 0) {
        // This is the case where the value is not a tstr/bstr... make sure that
        // that 0 was passed as |entrySize| in beginAddEntry().
        if (entryRemainingBytes_ != 0) {
            LOG(ERROR) << "Passed in non-zero entrySize " << entryRemainingBytes_
                       << " to beginAddEntry() but value passed to addEntryValue() is a tstr/bstr";
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Passed in non-zero entrySize %zd to beginAddEntry() but "
                                     "value passed to addEntryValue() is a tstr/bstr",
                                     entryRemainingBytes_),
                     {});
            return Void();
        }
    } else {
        // value is a tstr/bstr
        if (size_t(chunkSize) > IdentityCredentialStore::kGcmChunkSize) {
            _hidl_cb(support::result(
                         ResultCode::INVALID_DATA,
                         "Passed in chunk of size %zd is bigger than kGcmChunkSize which is %zd",
                         size_t(chunkSize), IdentityCredentialStore::kGcmChunkSize),
                     {});
            return Void();
        }
        if (size_t(chunkSize) > entryRemainingBytes_) {
            _hidl_cb(support::result(ResultCode::INVALID_DATA,
                                     "Passed in chunk of size %zd is bigger than remaining space "
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

    vector<uint8_t> cborData;
    if (!support::cborEncode(cborValue, cborData)) {
        cn_cbor_free(cborValue);
        _hidl_cb(support::result(ResultCode::FAILED, "Error encoding CBOR"), {});
        return Void();
    }
    cn_cbor_free(cborValue);

    vector<uint8_t> nonce;
    if (!support::getRandom(12, nonce)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error getting nonce"), {});
        return Void();
    }
    vector<uint8_t> encryptedContent;
    if (!support::encryptAes128Gcm(storageKey_, nonce, cborData, entryAdditionalData_,
                                   encryptedContent)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error encrypting content"), {});
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

    _hidl_cb(support::resultOK(), encryptedContent);
    return Void();
}

Return<void> WritableIdentityCredential::finishAddingEntries(finishAddingEntries_cb _hidl_cb) {
    vector<uint8_t> encodedCbor;
    if (!signedDataBuilder_.getEncodedCbor(encodedCbor)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error encoding CBOR"), {}, {});
        return Void();
    }

    vector<uint8_t> signature;
    if (!support::signEcDsa(credentialPrivKey_, encodedCbor, signature)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error signing data"), {}, {});
        return Void();
    }

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey_, credentialKeys)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error generating CredentialKeys"), {}, {});
        return Void();
    }

    vector<uint8_t> credentialData;
    if (!generateCredentialData(
            testCredential_ ? support::getTestHardwareBoundKey() : support::getHardwareBoundKey(),
            docType_, testCredential_, credentialKeys, credentialData)) {
        _hidl_cb(support::result(ResultCode::FAILED, "Error generating CredentialData"), {}, {});
        return Void();
    }

    _hidl_cb(support::resultOK(), credentialData, signature);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
