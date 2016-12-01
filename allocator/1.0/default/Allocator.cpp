#include <cutils/ashmem.h>
#include "Allocator.h"

namespace android {
namespace hardware {
namespace allocator {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::allocator::V1_0::IAllocator follow.
Return<void> Allocator::allocate(uint32_t size, const hidl_string& identifier, allocate_cb _hidl_cb) {
    int fd = ashmem_create_region(identifier.c_str(), size);
    if (fd >= 0) {
        native_handle_t* handle = native_handle_create(1, 0);
        handle->data[0] = fd;
        hidl_memory memory(handle, size);
        _hidl_cb(IAllocator::Status::OK, memory);
    } else {
        _hidl_cb(IAllocator::Status::FAILED, hidl_memory());
    }
    return Void();
}


IAllocator* HIDL_FETCH_IAllocator(const char* /* name */) {
    return new Allocator();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace allocator
}  // namespace hardware
}  // namespace android
