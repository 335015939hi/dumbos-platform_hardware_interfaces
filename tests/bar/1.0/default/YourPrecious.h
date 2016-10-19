#ifndef HIDL_GENERATED_android_hardware_tests_bar_V1_0_YourPrecious_H_
#define HIDL_GENERATED_android_hardware_tests_bar_V1_0_YourPrecious_H_

#include <android/hardware/tests/bar/1.0/IYourPrecious.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::bar::V1_0::IMyPrecious;
using ::android::hardware::tests::bar::V1_0::IYourPrecious;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct YourPrecious : public IYourPrecious {
    // Methods from ::android::hardware::tests::bar::V1_0::IMyPrecious follow.
    Return<void> myPrecious()  override;

    // Methods from ::android::hardware::tests::bar::V1_0::IYourPrecious follow.
    Return<void> dontYouTakeMyPrecious()  override;

};

extern "C" IYourPrecious* HIDL_FETCH_IYourPrecious(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_bar_V1_0_YourPrecious_H_
