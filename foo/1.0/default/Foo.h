#ifndef ANDROID_HARDWARE_FOO_V1_0_FOO_H
#define ANDROID_HARDWARE_FOO_V1_0_FOO_H

#include <android/hardware/foo/1.0/IFoo.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace foo {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Foo : public IFoo {
    // Methods from ::android::hardware::foo::V1_0::IFoo follow.
    Return<int32_t> foo(int32_t valueIn) override;
    Return<void> bar(int32_t valueIn, bar_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IFoo* HIDL_FETCH_IFoo(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FOO_V1_0_FOO_H
