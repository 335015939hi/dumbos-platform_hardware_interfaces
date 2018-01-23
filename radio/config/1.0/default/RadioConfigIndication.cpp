#include "RadioConfigIndication.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_0 {
namespace implementation {

using namespace ::android::hardware::radio::V1_0;

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfigIndication follow.
Return<void> RadioConfigIndication::simSlotsStatusChanged(
    RadioIndicationType /* type */, const hidl_vec<SimSlotStatus>& /* slotStatus */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
