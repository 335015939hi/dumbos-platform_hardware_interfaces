#include "keystore.h"

#include "hidl_return_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace keystore {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

// Methods from ::android::hardware::wifi::keystore::V1_0::IKeystore follow.
Return<void> Keystore::getBlob(const hidl_string& key, getBlob_cb _hidl_cb) {
    return validateAndCall(this,
                           KeystoreStatusCode::ERROR_UNKNOWN,
                           &Keystore::getBlobInternal,
                           _hidl_cb,
                           key);
}

Return<void> Keystore::getPublicKey(
        const hidl_string& keyId, getPublicKey_cb _hidl_cb) {
    return validateAndCall(this,
                           KeystoreStatusCode::ERROR_UNKNOWN,
                           &Keystore::getPublicKeyInternal,
                           _hidl_cb,
                           keyId);
}

Return<void> Keystore::sign(
        const hidl_string& keyId, const hidl_vec<uint8_t>& dataToSign,
        sign_cb _hidl_cb) {
    return validateAndCall(this,
                           KeystoreStatusCode::ERROR_UNKNOWN,
                           &Keystore::signInternal,
                           _hidl_cb,
                           keyId,
                           dataToSign);
}


// Worker functions.
std::pair<IKeystore::KeystoreStatusCode, std::vector<uint8_t>>
        Keystore::getPublicKeyInternal(sp<IKeystoreService> service,
                                       const hidl_string& keyId) {
    uint8_t *pubkey;
    size_t pubkeyLength;
    int ret = service->get_pubkey(String16(keyId), &pubkey, &pubkeyLength);
    if (ret != NO_ERROR) {
      return {KeystoreStatusCode::ERROR_UNKNOWN, {}};
    }

    std::vector<uint8_t> result(pubkey, pubkey + pubkeyLength);
    free(pubkey);
    return {KeystoreStatusCode::SUCCESS, result};
}

std::pair<IKeystore::KeystoreStatusCode, std::vector<uint8_t>>
        Keystore::getBlobInternal(sp<IKeystoreService> service,
                                  const hidl_string& key) {

    uint8_t *value;
    size_t valueLength;
    int ret = service->get(String16(key), -1, &value, &valueLength);
    if (ret != NO_ERROR) {
      return {KeystoreStatusCode::ERROR_UNKNOWN, {}};
    }

    std::vector<uint8_t> result(value, value + valueLength);
    free(value);
    return {KeystoreStatusCode::SUCCESS, result};
}

std::pair<IKeystore::KeystoreStatusCode, std::vector<uint8_t>>
        Keystore::signInternal(sp<IKeystoreService> service,
                               const hidl_string& keyId,
                               const std::vector<uint8_t>& dataToSign) {

    uint8_t *signedData;
    size_t signedDataLength;
    int ret = service->sign(
        String16(keyId), &dataToSign[0], dataToSign.size(), &signedData,
        &signedDataLength);
    if (ret != NO_ERROR) {
      return {KeystoreStatusCode::ERROR_UNKNOWN, {}};
    }

    std::vector<uint8_t> result(signedData, signedData + signedDataLength);
    free(signedData);
    return {KeystoreStatusCode::SUCCESS, result};
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IKeystore* HIDL_FETCH_IKeystore(const char* /* name */) {
    return new Keystore();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace keystore
}  // namespace wifi
}  // namespace hardware
}  // namespace android
