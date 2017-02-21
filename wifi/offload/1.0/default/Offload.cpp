#include "Offload.h"

namespace android {
namespace hardware {
namespace wifi {
namespace offload {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::wifi::offload::V1_0::IOffload follow.
Return<void> Offload::configureScans(const ScanParam& param, const ScanFilter& filter) {
    // TODO implement
    return Void();
}

Return<void> Offload::getScanStats(getScanStats_cb _hidl_cb) {
    // TODO implement
    return Void();
}

Return<void> Offload::subscribeScanResults(uint32_t delayMs) {
    // TODO implement
    return Void();
}

Return<void> Offload::unsubscribeScanResults() {
    // TODO implement
    return Void();
}

Return<void> Offload::setEventCallback(const sp<IOffloadCallback>& cb) {
    // TODO implement
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IOffload* HIDL_FETCH_IOffload(const char* /* name */) {
    return new Offload();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace offload
}  // namespace wifi
}  // namespace hardware
}  // namespace android
