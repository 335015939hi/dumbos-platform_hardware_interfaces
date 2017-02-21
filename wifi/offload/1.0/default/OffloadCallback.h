#ifndef ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOADCALLBACK_H
#define ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOADCALLBACK_H

#include <android/hardware/wifi/offload/1.0/IOffloadCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace wifi {
namespace offload {
namespace V1_0 {
namespace implementation {

using ::android::hardware::wifi::offload::V1_0::IOffloadCallback;
using ::android::hardware::wifi::offload::V1_0::ScanResult;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct OffloadCallback : public IOffloadCallback {
    // Methods from ::android::hardware::wifi::offload::V1_0::IOffloadCallback follow.
    Return<void> onScanResult(const hidl_vec<ScanResult>& scanResult) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

extern "C" IOffloadCallback* HIDL_FETCH_IOffloadCallback(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace offload
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_WIFI_OFFLOAD_V1_0_OFFLOADCALLBACK_H
