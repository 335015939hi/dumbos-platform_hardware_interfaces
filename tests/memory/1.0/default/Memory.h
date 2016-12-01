#ifndef ANDROID_HARDWARE_TESTS_MEMORY_V1_0__MEMORY_H
#define ANDROID_HARDWARE_TESTS_MEMORY_V1_0__MEMORY_H

#include <android/hardware/tests/memory/1.0/IMemory.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace tests {
namespace memory {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::memory::V1_0::IMemory;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Memory : public IMemory {
    // Methods from ::android::hardware::tests::memory::V1_0::IMemory follow.
    Return<void> fillMemory(const hidl_memory& memory_in, uint8_t filler) override;

};

extern "C" IMemory* HIDL_FETCH_IMemory(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace memory
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TESTS_MEMORY_V1_0__MEMORY_H
