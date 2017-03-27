#include "OemLock.h"

namespace android {
namespace hardware {
namespace oemlock {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
Return<OemLockSetUnlockAllowedByCarrierStatus> OemLock::setOemUnlockAllowedByCarrier(
        bool /* allowed */, const hidl_vec<uint8_t>& /* signature*/ ) {
    return OemLockSetUnlockAllowedByCarrierStatus::FAILED;
}

Return<void> OemLock::isOemUnlockAllowedByCarrier(isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::FAILED, true);
    return Void();
}

Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool /* allowed */) {
    return OemLockStatus::FAILED;
}

Return<void> OemLock::isOemUnlockAllowedByDevice(isOemUnlockAllowedByDevice_cb _hidl_cb) {
    _hidl_cb(OemLockStatus::FAILED, true);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace oemlock
}  // namespace hardware
}  // namespace android
