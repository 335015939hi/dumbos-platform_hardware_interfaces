#define LOG_TAG "danielnorman_lazy"

#include <android/hardware/tests/lazy/1.0/ILazy.h>

#include <utils/Log.h>
#include <utils/StrongPointer.h>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::tests::lazy::V1_0::ILazy;

int main(int /* argc */, char* /* argv */ []) {
    ::android::sp<ILazy> lazy = ILazy::getService();
    if (lazy == nullptr) {
        ALOGI("ILazy::getService() is nullptr");
        return 1;
    }
    Return status = lazy->sayHello(
            [&](const hidl_string message) { ALOGI("Message: %s", message.c_str()); });
    ALOGI("Return status: %s", status.description().c_str());
}
