#ifndef ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARC_H
#define ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARC_H

#include <algorithm>

#include <android/hardware/tv/earc/1.0/IHdmiEarc.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <hardware/hardware.h>
#include <hardware/hdmi_earc.h>

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
using ::android::hardware::tv::earc::V1_0::EarcStatus;
using ::android::hardware::tv::earc::V1_0::IHdmiEarc;
using ::android::hardware::tv::earc::V1_0::IHdmiEarcCallback;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;

struct HdmiEarc : public IHdmiEarc {
    HdmiEarc();
    // Methods from ::android::hardware::tv::earc::V1_0::IHdmiEarc follow.
    Return<void> setCallback(const sp<IHdmiEarcCallback>& callback) override;
    Return<EarcStatus> get_earc_status(int32_t portId) override;
    static void eventCallback(const hdmi_earc_event_t* event, void*) {
        if (mCallback != nullptr && event != nullptr) {
            if (event->type == EARC_STATUS_CHG) {
                hdmi_earc_event_t* tmp_event = const_cast<hdmi_earc_event_t*>(event);
                EarcEvent earcEvent{.status = static_cast<EarcStatus>(tmp_event->earc.status),
                                    .portId = static_cast<uint32_t>(tmp_event->earc.port_id)};
                mCallback->onEarcEvent(earcEvent);
            }
        }
    }

    // Methods from ::android::hidl::base::V1_0::IBase follow.
  private:
    static sp<IHdmiEarcCallback> mCallback;
    const hdmi_earc_device_t* mDevice;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace earc
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_EARC_V1_0_HDMIEARC_H
