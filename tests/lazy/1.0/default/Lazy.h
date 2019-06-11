#pragma once

#include <android/hardware/tests/lazy/1.0/ILazy.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace tests {
namespace lazy {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct Lazy : public ILazy {
    // Methods from ::android::hardware::tests::lazy::V1_0::ILazy follow.
    Return<void> sayHello(sayHello_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace lazy
}  // namespace tests
}  // namespace hardware
}  // namespace android
