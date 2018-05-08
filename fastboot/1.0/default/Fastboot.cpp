#include "Fastboot.h"

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setActiveSlot(
    const hidl_string& slot) {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<void> Fastboot::rebootBootloader() {
    // TODO implement
    return Void();
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setFlashingUnlock() {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<::android::hardware::fastboot::V1_0::Result> Fastboot::setFlashingLock() {
    // TODO implement
    return ::android::hardware::fastboot::V1_0::Result{};
}

Return<void> Fastboot::setFlashingUnlockCritical() {
    // TODO implement
    return Void();
}

Return<void> Fastboot::setFlashingLockCritical() {
    // TODO implement
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

// IFastboot* HIDL_FETCH_IFastboot(const char* /* name */) {
//    return new Fastboot();
//}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
