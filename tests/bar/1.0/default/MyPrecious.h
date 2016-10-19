#ifndef HIDL_GENERATED_android_hardware_tests_bar_V1_0_MyPrecious_H_
#define HIDL_GENERATED_android_hardware_tests_bar_V1_0_MyPrecious_H_

#include <android/hardware/tests/bar/1.0/IMyPrecious.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::bar::V1_0::IMyPrecious;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct MyPrecious : public IMyPrecious {
    // Methods from ::android::hardware::tests::bar::V1_0::IMyPrecious follow.
    Return<void> myPrecious()  override;

};

extern "C" IMyPrecious* HIDL_FETCH_IMyPrecious(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_bar_V1_0_MyPrecious_H_
