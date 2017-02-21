#ifndef ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOAD_H
#define ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOAD_H

#include <android/hardware/wifi/offload/1.0/IOffload.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace wifi {
namespace offload {
namespace V1_0 {
namespace implementation {

struct Offload : public IOffload {
    // Methods from ::android::hardware::wifi::offload::V1_0::IOffload follow.
    Return<void> configureScans(const ScanParam& param, const ScanFilter& filter) override;
    Return<void> getScanStats(getScanStats_cb _hidl_cb) override;
    Return<void> subscribeScanResults(uint32_t delayMs) override;
    Return<void> unsubscribeScanResults() override;
    Return<void> setEventCallback(const sp<IOffloadCallback>& cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

extern "C" IOffload* HIDL_FETCH_IOffload(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace offload
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOAD_H
