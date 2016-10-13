#ifndef HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_
#define HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>

#include <hidl/MQDescriptor.h>
#include "vendor_interface.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::hidl_vec;

class BluetoothHci : public IBluetoothHci {
 public:
  BluetoothHci();
  ~BluetoothHci();

  Return<Status> initialize(
      const ::android::sp<IBluetoothHciCallbacks>& cb) override;
  Return<void> sendHciCommand(const hidl_vec<uint8_t>& packet) override;
  Return<void> sendAclData(const hidl_vec<uint8_t>& data) override;
  Return<void> sendScoData(const hidl_vec<uint8_t>& data) override;
  Return<void> close() override;

 private:
  void sendDataToController(const uint8_t type, const hidl_vec<uint8_t>& data);
  ::android::sp<IBluetoothHciCallbacks> event_cb_;
  VendorInterface *vendor_interface_;
};

extern "C" IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_bluetooth_V1_0_BluetoothHci_H_
