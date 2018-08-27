//#define LOG_NDEBUG 0
#define LOG_TAG "FooService"

#include <android/hardware/foo/1.0/IFoo.h>
#include <hidl/LegacySupport.h>
#include "Foo.h"

using android::hardware::foo::V1_0::implementation::Foo;

int main(int /* argc */, char** /* argv */) {
    ::android::hardware::configureRpcThreadpool(1, true);
    auto foo = new Foo();
    if (foo == nullptr ||
        foo->registerAsService() != ::android::OK) {
        ALOGE("error register foo service!");
        return 1;
    }
    ::android::hardware::joinRpcThreadpool();
    return 1;   // joinRpcThreadPool should never return
}