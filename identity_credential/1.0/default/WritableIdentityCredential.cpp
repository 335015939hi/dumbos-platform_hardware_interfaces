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

#include <cn-cbor/cn-cbor.h>

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
using android::hardware::identity_credential::support::hexdump;

static vector<uint8_t> getTestHardwareBoundKey() {
    vector<uint8_t> HBK;
    HBK.resize(16);
    for (size_t n = 0; n < 16; n++) {
        HBK[n] = 0;
    }
    return HBK;
}

struct CnCborDeleter {
    void operator()(cn_cbor* ptr) {
        if (ptr != nullptr) {
            cn_cbor_free(ptr);
        }
    }
};

typedef std::unique_ptr<cn_cbor, CnCborDeleter> CnCborPtr;

static bool cborEncode(cn_cbor* value, vector<uint8_t>& encoded) {
    // Unfortunately there's no way to know how big the encoded blob will be [1]
    // so we just hardcode a ceiling of 16 KiB for now... it's not very elegant
    // but it works.
    //
    // [1] : ideally cn_cbor_encoded_write(nullptr, 0, 0, array.get()) would
    // return how many bytes _would_ have been written just like sprintf() and
    // friends... but that's not how it works right now.
    encoded.resize(16384);
    ssize_t enc_sz = cn_cbor_encoder_write(encoded.data(), 0, encoded.size(), value);
    if (enc_sz == -1) {
        LOG(ERROR) << "Error encoding CBOR data";
        return false;
    }
    encoded.resize(enc_sz);
    return true;
}

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

    // TODO: generate certificate and credentialPrivKey
    vector<uint8_t> certificate;
    vector<uint8_t> credentialPrivKey;

    // Initialize storageKey_ which will be used in multiple hwbinder calls.
    if (!getRandom(16, storageKey_)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    numAccessControlProfileRemaining_ = accessControlProfileCount;
    numEntriesRemaining_ = entryCount;

    vector<uint8_t> credentialKeys;
    if (!generateCredentialKeys(storageKey_, credentialPrivKey, credentialKeys)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }

    vector<uint8_t> credentialBlob;
    hexdump("credentialKeys", credentialKeys);

    vector<uint8_t> nonce;
    if (!getRandom(12, nonce)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    if (!encryptAes128Gcm(getTestHardwareBoundKey(), nonce, credentialKeys, {}, credentialBlob)) {
        _hidl_cb(ResultCode::FAILED, {}, {});
        return Void();
    }
    hexdump("credentialBlob", credentialBlob);

    vector<uint8_t> decryptedCredentialKeys;
    decryptAes128Gcm(getTestHardwareBoundKey(), credentialBlob, {}, decryptedCredentialKeys);
    hexdump("decrypted credentialBlob", decryptedCredentialKeys);

    _hidl_cb(ResultCode::OK, certificate, credentialBlob);
    return Void();
}

static bool cborMapPutStringInt(cn_cbor* map, const std::string& key, int64_t value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_int_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating int (pos " << err.pos << ")";
        return false;
    }
    if (!cn_cbor_mapput_string(map, key.c_str(), cbor_value, &err)) {
        LOG(ERROR) << "Error " << err.err << " putting value in map (pos " << err.pos << ")";
        cn_cbor_free(cbor_value);
        return false;
    }
    return true;
}

static bool cborMapPutStringBStr(cn_cbor* map, const std::string& key,
                                 const vector<uint8_t>& value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_data_create(value.data(), value.size(), &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    if (!cn_cbor_mapput_string(map, key.c_str(), cbor_value, &err)) {
        LOG(ERROR) << "Error " << err.err << " putting value in map (pos " << err.pos << ")";
        cn_cbor_free(cbor_value);
        return false;
    }
    return true;
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
        if (!cborMapPutStringBStr(map.get(), "readerAuthPubKey", profile.readerAuthPubKey)) {
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
    hexdump("profile mac", profile.mac);

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

    return ResultCode::OK;
}

Return<void> WritableIdentityCredential::addEntryValue(
    const ::android::hardware::identity_credential::V1_0::EntryValue& value,
    addEntryValue_cb _hidl_cb) {
    LOG(INFO) << "Entering " << __FUNCTION__;
    vector<uint8_t> encrypted_content;
    _hidl_cb(ResultCode::OK, encrypted_content);
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
