#ifndef ANDROID_HARDWARE_STORAGEKEY_V1_0_STORAGEKEY_H
#define ANDROID_HARDWARE_STORAGEKEY_V1_0_STORAGEKEY_H

#include <android/hardware/storagekey/1.0/IStorageKey.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace storagekey {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct StorageKey : public IStorageKey {
    // Methods from ::android::hardware::storagekey::V1_0::IStorageKey follow.
    Return<void> generateStorageKey(const hidl_vec<KeyParameter>& keyParams,
                                    generateStorageKey_cb _hidl_cb) override;

    Return<void> importStorageKey(const hidl_vec<KeyParameter>& keyParams,
                                  const hidl_vec<uint8_t>& keyData,
                                  importStorageKey_cb _hidl_cb) override;

    Return<void> exportStorageKey(const hidl_vec<uint8_t>& keyBlob,
                                  exportStorageKey_cb _hidl_cb) override;

    Return<void> deleteStorageKey(const hidl_vec<uint8_t>& keyBlob,
                                  deleteStorageKey_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace storagekey
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_STORAGEKEY_V1_0_STORAGEKEY_H
