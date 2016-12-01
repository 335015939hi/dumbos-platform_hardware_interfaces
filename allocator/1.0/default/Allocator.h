#ifndef ANDROID_HARDWARE_ALLOCATOR_V1_0__ALLOCATOR_H
#define ANDROID_HARDWARE_ALLOCATOR_V1_0__ALLOCATOR_H

#include <android/hardware/allocator/1.0/IAllocator.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace allocator {
namespace V1_0 {
namespace implementation {

using ::android::hardware::allocator::V1_0::IAllocator;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Allocator : public IAllocator {
    // Methods from ::android::hardware::allocator::V1_0::IAllocator follow.
    Return<void> allocate(uint32_t size, const hidl_string& identifier, allocate_cb _hidl_cb) override;

};

extern "C" IAllocator* HIDL_FETCH_IAllocator(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace allocator
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_ALLOCATOR_V1_0__ALLOCATOR_H
