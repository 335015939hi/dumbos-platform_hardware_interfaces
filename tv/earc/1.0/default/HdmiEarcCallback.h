#ifndef ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARCCALLBACK_H
#define ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARCCALLBACK_H

#include <android/hardware/tv/earc/1.0/IHdmiEarcCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace tv {
namespace earc {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tv::earc::V1_0::EarcEvent;
using ::android::hardware::tv::earc::V1_0::IHdmiEarcCallback;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;

struct HdmiEarcCallback : public IHdmiEarcCallback {
    // Methods from ::android::hardware::tv::earc::V1_0::IHdmiEarcCallback follow.
    Return<void> onEarcEvent(const EarcEvent& event) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

extern "C" IHdmiEarcCallback* HIDL_FETCH_IHdmiEarcCallback(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace earc
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARCCALLBACK_H
