#define LOG_TAG "hidl_test"
#include <android-base/logging.h>

#include "MyPrecious.h"

namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::bar::V1_0::IMyPrecious follow.
Return<void> MyPrecious::myPrecious()  {
    ALOGI("SERVER(Bar) MyPrecious::myPrecious");
    return Void();
}


IMyPrecious* HIDL_FETCH_IMyPrecious(const char* /* name */) {
    return new MyPrecious();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android
