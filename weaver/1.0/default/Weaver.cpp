#include "Weaver.h"

namespace android {
namespace hardware {
namespace weaver {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
Return<void> Weaver::getConfig(getConfig_cb _hidl_cb) {
    _hidl_cb(WeaverStatus::FAILED, WeaverConfig{});
    return Void();
}

Return<WeaverStatus> Weaver::write(uint32_t /* slotId */, const hidl_vec<uint8_t>& /* key */,
                                   const hidl_vec<uint8_t>& /* value */) {
    return WeaverStatus::FAILED;
}

Return<void> Weaver::read(uint32_t /* slotId */, const hidl_vec<uint8_t>& /* key */,
                          read_cb _hidl_cb) {
    _hidl_cb(WeaverReadStatus::FAILED, WeaverReadResponse{});
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace weaver
}  // namespace hardware
}  // namespace android
