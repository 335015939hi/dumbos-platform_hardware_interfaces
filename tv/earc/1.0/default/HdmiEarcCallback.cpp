#include "HdmiEarcCallback.h"

namespace android {
namespace hardware {
namespace tv {
namespace earc {
namespace V1_0 {
namespace implementation {

// Methods from ::android:::hardware::tv::earc::V1_0::IHdmiEarcCallback follow.
Return<void> HdmiEarcCallback::onEarcEvent(const EarcEvent& event) {
    // TODO implement
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IHdmiEarcCallback* HIDL_FETCH_IHdmiEarcCallback(const char* /* name */) {
    return new HdmiEarcCallback();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace earc
}  // namespace tv
}  // namespace hardware
}  // namespace android
