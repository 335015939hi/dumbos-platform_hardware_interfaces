#ifndef ANDROID_HARDWARE_CONFIGSTORE_V1_1_CHARGERCONFIGS_H
#define ANDROID_HARDWARE_CONFIGSTORE_V1_1_CHARGERCONFIGS_H

#include <android/hardware/configstore/1.1/IChargerConfigs.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_1 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct ChargerConfigs : public IChargerConfigs {
    // Methods from ::android::hardware::configstore::V1_1::IChargerConfigs follow.
    Return<void> drawSplitScreen(drawSplitScreen_cb _hidl_cb) override;
    Return<void> drawSplitOffset(drawSplitOffset_cb _hidl_cb) override;
    Return<void> disableInitBlank(disableInitBlank_cb _hidl_cb) override;
    Return<void> enableSuspend(enableSuspend_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

}  // namespace implementation
}  // namespace V1_1
}  // namespace configstore
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONFIGSTORE_V1_1_CHARGERCONFIGS_H
