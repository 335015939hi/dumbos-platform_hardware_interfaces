#ifndef ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H
#define ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H

#include <android/hardware/radio/config/1.0/IRadioConfigIndication.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct RadioConfigIndication : public IRadioConfigIndication {
    // Methods from ::android::hardware::radio::config::V1_0::IRadioConfigIndication follow.
    Return<void> simSlotsStatusChanged(
        ::android::hardware::radio::V1_0::RadioIndicationType type,
        const hidl_vec<::android::hardware::radio::config::V1_0::SimSlotStatus>& slotStatus)
        override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H
