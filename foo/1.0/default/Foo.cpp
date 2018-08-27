#include "Foo.h"

namespace android {
namespace hardware {
namespace foo {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::foo::V1_0::IFoo follow.
Return<int32_t> Foo::foo(int32_t valueIn) {
    (void)valueIn;
    return int32_t {};
}

Return<void> Foo::bar(int32_t valueIn, bar_cb _hidl_cb) {
    // TODO implement
    _hidl_cb(valueIn, false);
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IFoo* HIDL_FETCH_IFoo(const char* /* name */) {
    //return new Foo();
//}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace hardware
}  // namespace android
