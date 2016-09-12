#ifndef HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHciEvent_H_
#define HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHciEvent_H_

#include <android/hardware/bluetooth/1.0/IBluetoothHciEvent.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::bluetooth::V1_0::IBluetoothHciEvent;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct BluetoothHciEvent : public IBluetoothHciEvent {
    // Methods from ::android::hardware::bluetooth::V1_0::IBluetoothHciEvent follow.
    Return<void> sendHciEvent(const hidl_vec<uint8_t>& event)  override;

};

extern "C" IBluetoothHciEvent* HIDL_FETCH_IBluetoothHciEvent(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHciEvent_H_
