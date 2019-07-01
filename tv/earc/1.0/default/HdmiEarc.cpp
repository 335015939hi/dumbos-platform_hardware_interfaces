#include <android-base/logging.h>

#include <hardware/hardware.h>
#include <hardware/hdmi_earc.h>

#include "HdmiEarc.h"

namespace android {
namespace hardware {
namespace tv {
namespace earc {
namespace V1_0 {
namespace implementation {

sp<IHdmiEarcCallback> HdmiEarc::mCallback = nullptr;

HdmiEarc::HdmiEarc() {
    hdmi_earc_device_t* hdmi_earc_device = nullptr;
    int ret = 0;
    const hw_module_t* earc_hw_module = nullptr;

    // TODO : implement driver open module
    //    ret = hw_get_module(HDMI_EARC_HARDWARE_MODULE_ID, &earc_hw_module);
    //    if (ret != 0) {
    //        LOG(ERROR) << "hw_get_earc_module failed: " << ret;
    //    } else {
    //
    //        ret = hdmi_earc_open(earc_hw_module, &hdmi_earc_device);
    //        if (ret != 0) {
    //            LOG(ERROR) << "hdmi_earc_open failed: " << ret;
    //        } else {
    //            LOG(ERROR) << "hw_get_earc_module success: " << ret;
    //            mDevice = hdmi_earc_device;
    //        }
    //    }
}

// Methods from ::android::hardware::tv::earc::V1_0::IHdmiEarc follow.
Return<void> HdmiEarc::setCallback(const sp<IHdmiEarcCallback>& callback) {
    mCallback = callback;
    mDevice->register_event_callback(mDevice, eventCallback, nullptr);
    return Void();
}

Return<EarcStatus> HdmiEarc::get_earc_status(int32_t portId) {
    return static_cast<EarcStatus>(mDevice->get_earc_status(mDevice, portId));
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace earc
}  // namespace tv
}  // namespace hardware
}  // namespace android
