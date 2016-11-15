#include "DumpstateDevice.h"

#include <log/log.h>

namespace android {
namespace hardware {
namespace dumpstate {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::dumpstate::V1_0::IDumpstateDevice follow.

Return<void> DumpstateDevice::dumpstateBoard(const sp<IDumper>& /* dumper */) {
    LOG_ALWAYS_FATAL("Should not be instantiated");
    return Void();
}

IDumpstateDevice* HIDL_FETCH_IDumpstateDevice(const char* /* name */) {
    // There is no default implementation.
    return nullptr;
}

} // namespace implementation
}  // namespace V1_0
}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
