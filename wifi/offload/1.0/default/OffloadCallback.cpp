#include "OffloadCallback.h"

namespace android {
namespace hardware {
namespace wifi {
namespace offload {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::wifi::offload::V1_0::IOffloadCallback follow.
Return<void> OffloadCallback::onScanResult(const hidl_vec<ScanResult>& scanResult) {
    // TODO implement
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IOffloadCallback* HIDL_FETCH_IOffloadCallback(const char* /* name */) {
    return new OffloadCallback();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace offload
}  // namespace wifi
}  // namespace hardware
}  // namespace android
