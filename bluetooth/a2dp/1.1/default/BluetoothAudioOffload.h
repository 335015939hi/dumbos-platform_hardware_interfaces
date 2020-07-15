// FIXME: your file license if you have one

#pragma once

#include <android/hardware/bluetooth/a2dp/1.1/IBluetoothAudioOffload.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace a2dp {
namespace V1_1 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct BluetoothAudioOffload : public IBluetoothAudioOffload {
    // Methods from ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.
    Return<::android::hardware::bluetooth::a2dp::V1_0::Status> startSession(const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf, const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration& codecConfig) override;
    Return<void> streamStarted(::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
    Return<void> streamSuspended(::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
    Return<void> endSession() override;

    // Methods from ::android::hardware::bluetooth::a2dp::V1_1::IBluetoothAudioOffload follow.
    Return<::android::hardware::bluetooth::a2dp::V1_1::Status> startSession_1_1(const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf, const ::android::hardware::bluetooth::a2dp::V1_1::CodecConfiguration& codecConfig) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IBluetoothAudioOffload* HIDL_FETCH_IBluetoothAudioOffload(const char* name);

}  // namespace implementation
}  // namespace V1_1
}  // namespace a2dp
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
