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

#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>
#include <android/hardware/keymaster/capability/1.0/types.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace implementation {

using std::vector;

using android::hardware::identity_credential::support::CnCborPtr;
using android::hardware::identity_credential::support::decryptAes128Gcm;
using android::hardware::identity_credential::support::encryptAes128Gcm;
using android::hardware::identity_credential::support::getRandom;
using android::hardware::identity_credential::support::getTestHardwareBoundKey;
using android::hardware::identity_credential::support::hexdump;

using android::hardware::identity_credential::support::cborArrayAppendInt;
using android::hardware::identity_credential::support::cborArrayAppendValue;
using android::hardware::identity_credential::support::cborEncode;
using android::hardware::identity_credential::support::cborMapPutStringBStr;
using android::hardware::identity_credential::support::cborMapPutStringInt;
using android::hardware::identity_credential::support::cborMapPutStringString;
using android::hardware::identity_credential::support::cborMapPutStringValue;

using android::hardware::identity_credential::support::entryCreateAdditionalData;

using android::hardware::identity_credential::support::createEcKeyAndAttestationChain;

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
    auto array = CnCborPtr(cn_cbor_array_create(&err));
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

    if (!cborEncode(array.get(), credentialKeys)) {
        return false;
    }

    return true;
}

Return<void> WritableIdentityCredential::startPersonalization(
    const hidl_vec<uint8_t>& attestationApplicationId,
    const hidl_vec<uint8_t>& attestationChallenge, uint8_t accessControlProfileCount,
    uint16_t entryCount, startPersonalization_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;

    vector<uint8_t> certificate;
    vector<uint8_t> credentialPrivKey;
    if (!createEcKeyAndAttestationChain(credentialPrivKey, certificate)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    // Initialize storageKey_ which will be used in multiple hwbinder calls.
    if (!getRandom(16, storageKey_)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    // hexdump("storageKey", storageKey_);

    numAccessControlProfileRemaining_ = accessControlProfileCount;
    numEntriesRemaining_ = entryCount;
    accessControlProfiles_.resize(0);

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey, credentialKeys)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> credentialBlob;
    // hexdump("credentialKeys", credentialKeys);

    vector<uint8_t> nonce;
    if (!getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    if (!encryptAes128Gcm(getTestHardwareBoundKey(), nonce, credentialKeys, {}, credentialBlob)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    // hexdump("credentialBlob", credentialBlob);

    _hidl_cb(ResultCode::OK, certificate, credentialBlob);
    return Void();
}

static bool secureAccessControlProfileCalcMac(const SecureAccessControlProfile& profile,
                                              const vector<uint8_t>& storageKey,
                                              vector<uint8_t>& mac) {
    cn_cbor_errback err;

    auto map = CnCborPtr(cn_cbor_map_create(&err));
    if (map.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringInt(map.get(), "id", profile.id)) {
        return false;
    }

    if (profile.readerAuthPubKey.size() > 0) {
        if (!cborMapPutStringBStr(map.get(), "readerAuthPubKey", profile.readerAuthPubKey.data(),
                                  profile.readerAuthPubKey.size())) {
            return false;
        }
    }

    if (profile.capabilityId != 0) {
        if (!cborMapPutStringInt(map.get(), "capabilityId", profile.capabilityId)) {
            return false;
        }
    }
    if (profile.capabilityType !=
        ::android::hardware::keymaster::capability::V1_0::CapabilityType::NOT_APPLICABLE) {
        if (!cborMapPutStringInt(map.get(), "capabilityType", int64_t(profile.capabilityType))) {
            return false;
        }
    }

    vector<uint8_t> cborData;
    if (!cborEncode(map.get(), cborData)) {
        return false;
    }
    vector<uint8_t> nonce;
    if (!getRandom(12, nonce)) {
        return false;
    }
    if (!encryptAes128Gcm(storageKey, nonce, {}, cborData, mac)) {
        return false;
    }

    return true;
}

Return<void> WritableIdentityCredential::addAccessControlProfile(
    uint8_t id, const hidl_vec<uint8_t>& readerAuthPubKey, uint64_t capabilityId,
    CapabilityType capabilityType, uint32_t timeout, addAccessControlProfile_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;

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
    // hexdump("profile mac", profile.mac);
    accessControlProfiles_.push_back(profile);

    numAccessControlProfileRemaining_--;

    _hidl_cb(ResultCode::OK, profile);
    return Void();
}

Return<ResultCode> WritableIdentityCredential::beginAddEntry(
    const hidl_vec<SecureAccessControlProfile>& accessControlProfiles, const hidl_string& nameSpace,
    const hidl_string& name, bool directlyAvailable, uint32_t entrySize) {
    LOG(INFO) << "Entering " << __FUNCTION__;

    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        return ResultCode::FAILED;
    }

    if (numEntriesRemaining_ == 0) {
        LOG(ERROR) << "numEntriesReamining_ is 0 and expected non-zero";
        return ResultCode::FAILED;
    }

    // TODO: validate |accessControlProfiles|

    // TODO: should the HAL should pass a vector of ids instead of full profiles?
    vector<uint8_t> profileIds;
    profileIds.resize(accessControlProfiles.size());
    for (auto& profile : accessControlProfiles) {
        profileIds.push_back(profile.id);
    }
    if (!entryCreateAdditionalData(nameSpace, name, profileIds, entryAdditionalData_)) {
        return ResultCode::FAILED;
    }
    // hexdump("entryAdditionalData_", entryAdditionalData_);

    entryRemainingBytes_ = entrySize;

    return ResultCode::OK;
}

Return<void> WritableIdentityCredential::addEntryValue(const EntryValue& value,
                                                       addEntryValue_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;

    if (entryRemainingBytes_ == 0) {
        LOG(ERROR) << "entryRemainingBytes_ is " << entryRemainingBytes_ << " and expected zero";
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    cn_cbor* cbor_value;
    cn_cbor_errback err;
    switch (value.getDiscriminator()) {
        case EntryValue::hidl_discriminator::integer:
            cbor_value = cn_cbor_int_create(value.integer(), &err);
            break;
        case EntryValue::hidl_discriminator::textString:
            cbor_value = cn_cbor_string_create(value.textString().c_str(), &err);
            break;
        case EntryValue::hidl_discriminator::byteString:
            cbor_value =
                cn_cbor_data_create(value.byteString().data(), value.byteString().size(), &err);
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
    vector<uint8_t> cborData;
    if (!cborEncode(cbor_value, cborData)) {
        cn_cbor_free(cbor_value);
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    cn_cbor_free(cbor_value);

    vector<uint8_t> nonce;
    if (!getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }
    vector<uint8_t> encryptedContent;
    if (!encryptAes128Gcm(storageKey_, nonce, cborData, entryAdditionalData_, encryptedContent)) {
        _hidl_cb(ResultCode::FAILED, {});
        return Void();
    }

    hexdump("cborData", cborData);
    // hexdump("encryptedContent", encryptedContent);

    _hidl_cb(ResultCode::OK, encryptedContent);
    return Void();
}

Return<void> WritableIdentityCredential::finishAddingEntryies(finishAddingEntryies_cb _hidl_cb) {
    vector<uint8_t> signature;
    LOG(INFO) << "Entering " << __FUNCTION__;
    _hidl_cb(ResultCode::OK, signature);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
