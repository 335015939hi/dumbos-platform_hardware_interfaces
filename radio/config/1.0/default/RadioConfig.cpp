#include "RadioConfig.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_0 {
namespace implementation {

using namespace ::android::hardware::radio::config::V1_0;

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfig follow.
Return<void> RadioConfig::setResponseFunctions(
    const sp<IRadioConfigResponse>& radioConfigResponse,
    const sp<IRadioConfigIndication>& radioConfigIndication) {
    mRadioConfigResponse = radioConfigResponse;
    mRadioConfigIndication = radioConfigIndication;
    return Void();
}

Return<void> RadioConfig::getSimSlotsStatus(int32_t /* serial */) {
    hidl_vec<SimSlotStatus> slotStatus;
    ::android::hardware::radio::V1_0::RadioResponseInfo info;
    mRadioConfigResponse->getSimSlotsStatusResponse(info, slotStatus);
    return Void();
}

Return<void> RadioConfig::setSimSlotsMapping(int32_t /* serial */,
                                             const hidl_vec<uint32_t>& /* slotMap */) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;
    mRadioConfigResponse->setSimSlotsMappingResponse(info);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
