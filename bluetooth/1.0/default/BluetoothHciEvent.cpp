#include "BluetoothHciEvent.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::bluetooth::V1_0::IBluetoothHciEvent follow.
Return<void> BluetoothHciEvent::sendHciEvent(const hidl_vec<uint8_t>& event)  {
    // TODO implement
    return Void();
}


IBluetoothHciEvent* HIDL_FETCH_IBluetoothHciEvent(const char* /* name */) {
    return new BluetoothHciEvent();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
