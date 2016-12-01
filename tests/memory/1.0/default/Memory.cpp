#include "Memory.h"

namespace android {
namespace hardware {
namespace tests {
namespace memory {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::memory::V1_0::IMemory follow.
Return<void> Memory::fillMemory(const hidl_memory& memory_in, uint8_t filler) {
    uint8_t *memory = static_cast<uint8_t*>(memory_in.getPointer());
    if (memory != nullptr) {
        for (size_t i = 0; i < memory_in.size(); i++) {
            memory[i] = filler;
        }
    } else {
        ALOGE("Could not getPointer() from hidl_memory.");
    }
    return Void();
}


IMemory* HIDL_FETCH_IMemory(const char* /* name */) {
    return new Memory();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace memory
}  // namespace tests
}  // namespace hardware
}  // namespace android
