#ifndef ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H
#define ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H

#include <android/hardware/fastboot/1.0/IFastboot.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Fastboot : public IFastboot {
    // Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
    Return<::android::hardware::fastboot::V1_0::Result> setActiveSlot(
        const hidl_string& slot) override;
    Return<void> rebootBootloader() override;
    Return<::android::hardware::fastboot::V1_0::Result> setFlashingUnlock() override;
    Return<::android::hardware::fastboot::V1_0::Result> setFlashingLock() override;
    Return<void> setFlashingUnlockCritical() override;
    Return<void> setFlashingLockCritical() override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FASTBOOT_V1_0_FASTBOOT_H
