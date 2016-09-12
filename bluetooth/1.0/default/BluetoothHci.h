#ifndef HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_
#define HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>

#include <atomic>
#include <thread>

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::bluetooth::V1_0::IBluetoothHci;
using ::android::hardware::bluetooth::V1_0::IBluetoothHciEvent;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

/*
 * HCI packets indicate their type in the first byte of the packet.
 *
 * Command, ACL, Synchronous, and Event packet types are defined in the spec:
 * BLUETOOTH SPECIFICATION Version 4.2 [Vol 4, Part A] Table 2.1
 */
const uint8_t HCI_TYPE_EVENT = 4;

struct BluetoothHci : public IBluetoothHci {
    // Transport-dependent code
    int initialize();
    void cleanUp();
    void transmitPacket(const hidl_vec<uint8_t>& packet);
    void watchForEvents();
    void readEvent();

    // Methods from ::android::hardware::bluetooth::V1_0::IBluetoothHci follow.
    Return<void> sendHci(const hidl_vec<uint8_t>& packet)  override;
    Return<void> registerEventCb(const sp<IBluetoothHciEvent>& cb)  override;

    sp<IBluetoothHciEvent> eventInterface;

    bool initialized;

    // Transport-dependent state
    int fd;
    std::thread eventThread;
    std::atomic_bool thread_running;
   
};

extern "C" IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_
