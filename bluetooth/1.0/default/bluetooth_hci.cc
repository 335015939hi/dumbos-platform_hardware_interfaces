#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <utils/Log.h>

#include "bluetooth_hci.h"
#include "vendor_interface.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

static const uint8_t HCI_DATA_TYPE_COMMAND = 1;
static const uint8_t HCI_DATA_TYPE_ACL = 2;
static const uint8_t HCI_DATA_TYPE_SCO = 3;

Return<Status> BluetoothHci::initialize(
    const ::android::sp<IBluetoothHciCallbacks>& cb) {
  ALOGW("BluetoothHci::initialize()");
  event_cb_ = cb;

  bool rc = VendorInterface::Initialize(
      [this](HciPacketType type, const hidl_vec<uint8_t>& packet) {
        switch (type) {
          case HCI_PACKET_TYPE_EVENT:
            event_cb_->hciEventReceived(packet);
            break;
          case HCI_PACKET_TYPE_ACL_DATA:
            event_cb_->aclDataReceived(packet);
            break;
          case HCI_PACKET_TYPE_SCO_DATA:
            event_cb_->scoDataReceived(packet);
            break;
          default:
            ALOGE("%s Unexpected event type %d", __func__, type);
            break;
        }
      });
  if (!rc) return Status::INITIALIZATION_ERROR;

  return Status::SUCCESS;
}

Return<void> BluetoothHci::close() {
  ALOGW("BluetoothHci::close()");
  VendorInterface::Shutdown();
  return Void();
}

Return<void> BluetoothHci::sendHciCommand(const hidl_vec<uint8_t>& command) {
  sendDataToController(HCI_DATA_TYPE_COMMAND, command);
  return Void();
}

Return<void> BluetoothHci::sendAclData(const hidl_vec<uint8_t>& data) {
  sendDataToController(HCI_DATA_TYPE_ACL, data);
  return Void();
}

Return<void> BluetoothHci::sendScoData(const hidl_vec<uint8_t>& data) {
  sendDataToController(HCI_DATA_TYPE_SCO, data);
  return Void();
}

void BluetoothHci::sendDataToController(const uint8_t type,
                                        const hidl_vec<uint8_t>& data) {
  VendorInterface::get()->Send(&type, 1);
  VendorInterface::get()->Send(data.data(), data.size());
}

IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* /* name */) {
  return new BluetoothHci();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
