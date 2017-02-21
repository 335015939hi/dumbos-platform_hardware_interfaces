#include "Offload.h"
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace wifi {
namespace offload {
namespace V1_0 {
namespace implementation {

Offload::Offload() :
        mScanEventSubscription(false),
        mSubscriptionTimeMs(0),
        mSubscriptionDelayMs(0) {
    // Load Nano app
    LOG(android::base::INFO) << "Constructing Offoad";
}

// Methods from ::android::hardware::wifi::offload::V1_0::IOffload follow.
Return<void> Offload::configureScans(const ScanParam& param, const ScanFilter& filter) {
    Offload::mScanParam = param;
    Offload::mScanFilter = filter;
    // TODO: implement Wifi Nano app scan configuration
    return Void();
}

Return<void> Offload::getScanStats(getScanStats_cb offloadScanStatsCallback) {
    ScanStats *scanStats = nullptr;
    // TODO: implement getting scan stats from Wifi Nano app
    if (scanStats != nullptr) {
        offloadScanStatsCallback(*scanStats);
    }
    return Void();
}

Return<void> Offload::subscribeScanResults(uint32_t delayMs) {
    mScanEventSubscription = false;
    // TODO: get current time from Wifi Nano App
    mSubscriptionTimeMs = 0;
    mSubscriptionDelayMs = delayMs;
    // TODO implement informing Wifi Nano App
    return Void();
}

Return<void> Offload::unsubscribeScanResults() {
    mScanEventSubscription = false;
    // TODO: implement updating Wifi Nano app
    return Void();
}

Return<void> Offload::setEventCallback(const sp<IOffloadCallback>& cb) {
    Offload::mScanEventCallback = cb;
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_0
}  // namespace offload
}  // namespace wifi
}  // namespace hardware
}  // namespace android
