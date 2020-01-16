#include "StorageKey.h"

namespace android {
namespace hardware {
namespace storagekey {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::storagekey::V1_0::IStorageKey follow.
Return<void> StorageKey::generateStorageKey(const hidl_vec<KeyParameter>& keyParams,
                                            generateStorageKey_cb _hidl_cb) {
    (void)keyParams;
    (void)_hidl_cb;

    return Void();
}

Return<void> StorageKey::importStorageKey(const hidl_vec<KeyParameter>& keyParams,
                                          const hidl_vec<uint8_t>& keyData,
                                          importStorageKey_cb _hidl_cb) {
    (void)keyParams;
    (void)keyData;
    (void)_hidl_cb;

    return Void();
}

Return<void> StorageKey::exportStorageKey(const hidl_vec<uint8_t>& keyBlob,
                                          exportStorageKey_cb _hidl_cb) {
    (void)keyBlob;
    (void)_hidl_cb;

    return Void();
}

Return<ErrorCode> StorageKey::deleteStorageKey(const hidl_vec<uint8_t>& keyBlob) {
    (void)keyBlob;

    return ErrorCode::UNIMPLEMENTED;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace storagekey
}  // namespace hardware
}  // namespace android
