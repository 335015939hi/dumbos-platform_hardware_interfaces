#ifndef ANDROID_HARDWARE_WEAVER_V1_0_WEAVER_H
#define ANDROID_HARDWARE_WEAVER_V1_0_WEAVER_H

#include <android/hardware/weaver/1.0/IWeaver.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace weaver {
namespace V1_0 {
namespace implementation {

using ::android::hardware::weaver::V1_0::IWeaver;
using ::android::hardware::weaver::V1_0::WeaverConfig;
using ::android::hardware::weaver::V1_0::WeaverReadResponse;
using ::android::hardware::weaver::V1_0::WeaverReadStatus;
using ::android::hardware::weaver::V1_0::WeaverStatus;
using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Weaver : public IWeaver {
    // Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
    Return<void> getConfig(getConfig_cb _hidl_cb) override;
    Return<WeaverStatus> write(uint32_t slotId, const hidl_vec<uint8_t>& key,
                               const hidl_vec<uint8_t>& value) override;
    Return<void> read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace weaver
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_WEAVER_V1_0_WEAVER_H
