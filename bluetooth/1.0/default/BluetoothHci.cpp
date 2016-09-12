#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <utils/Log.h>

#include "BluetoothHci.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

Return<Status> BluetoothHci::initialize(
    const ::android::sp<IBluetoothHciCallback>& cb) {
  ALOGW("BluetoothHci::initialize()");
  event_cb_ = cb;
  cb->hciEventReceived(hidl_vec<uint8_t>());
  return Status::SUCCESS;
}

Return<void> BluetoothHci::close() {
  return Void();
}

Return<void> BluetoothHci::sendHciCommand(const hidl_vec<uint8_t>& command) {
  UNUSED(command);
  return Void();
}

Return<void> BluetoothHci::sendAclData(const hidl_vec<uint8_t>& data) {
  UNUSED(data);
  return Void();
}

Return<void> BluetoothHci::sendScoData(const hidl_vec<uint8_t>& data) {
  UNUSED(data);
  return Void();
}

IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* /* name */) {
  return new BluetoothHci();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
