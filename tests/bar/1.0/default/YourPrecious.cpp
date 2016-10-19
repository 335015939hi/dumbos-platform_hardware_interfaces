#define LOG_TAG "hidl_test"
#include <android-base/logging.h>

#include "YourPrecious.h"

namespace android {
namespace hardware {
namespace tests {
namespace bar {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::bar::V1_0::IMyPrecious follow.
Return<void> YourPrecious::myPrecious()  {
    ALOGI("SERVER(Bar) YourPrecious::myPrecious");
    return Void();
}


// Methods from ::android::hardware::tests::bar::V1_0::IYourPrecious follow.
Return<void> YourPrecious::dontYouTakeMyPrecious()  {
    ALOGI("SERVER(Bar) YourPrecious::dontYouTakeMyPrecious");
    return Void();
}


IYourPrecious* HIDL_FETCH_IYourPrecious(const char* /* name */) {
    return new YourPrecious();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace bar
}  // namespace tests
}  // namespace hardware
}  // namespace android
