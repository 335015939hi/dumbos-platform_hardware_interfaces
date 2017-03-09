#ifndef ANDROID_HARDWARE_WIFI_KEYSTORE_V1_0_KEYSTORE_H
#define ANDROID_HARDWARE_WIFI_KEYSTORE_V1_0_KEYSTORE_H

#include <android/hardware/wifi/keystore/1.0/IKeystore.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <keystore/IKeystoreService.h>
#include <binder/IServiceManager.h>

namespace android {
namespace hardware {
namespace wifi {
namespace keystore {
namespace V1_0 {
namespace implementation {

using ::android::hardware::wifi::keystore::V1_0::IKeystore;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Keystore : public IKeystore {
  public:
    // Methods from ::android::hardware::wifi::keystore::V1_0::IKeystore follow.
    Return<void> getBlob(const hidl_string& key, getBlob_cb _hidl_cb) override;
    Return<void> getPublicKey(
            const hidl_string& keyId, getPublicKey_cb _hidl_cb) override;
    Return<void> sign(
            const hidl_string& keyId, const hidl_vec<uint8_t>& dataToSign,
            sign_cb _hidl_cb) override;

    // Corresponding worker functions for the HIDL methods.
    std::pair<KeystoreStatusCode, std::vector<uint8_t>> getPublicKeyInternal(
            sp<IKeystoreService> service, const hidl_string& keyId);
    std::pair<KeystoreStatusCode, std::vector<uint8_t>> getBlobInternal(
            sp<IKeystoreService> service, const hidl_string& key);
    std::pair<KeystoreStatusCode, std::vector<uint8_t>> signInternal(
            sp<IKeystoreService> service, const hidl_string& key,
            const std::vector<uint8_t>& dataToSign);

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace keystore
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_WIFI_KEYSTORE_V1_0_KEYSTORE_H
