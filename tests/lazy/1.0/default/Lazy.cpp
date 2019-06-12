#include "Lazy.h"

#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>

namespace android {
namespace hardware {
namespace tests {
namespace lazy {
namespace V1_0 {
namespace implementation {

// Methods from ::android::frameworks::lazy::V1_0::ILazy follow.
Return<void> Lazy::sayHello(sayHello_cb _hidl_cb) {
    hidl_string message = "Hello, world!";
    _hidl_cb(message);

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace lazy
}  // namespace tests
}  // namespace hardware
}  // namespace android

int main(int /* argc */, char* /* argv */ []) {
    // This function must be called before you join to ensure the proper
    // number of threads are created. The threadpool will never exceed
    // size one because of this call.
    ::android::hardware::configureRpcThreadpool(1 /*threads*/, true /*willJoin*/);

    ::android::sp lazy = new ::android::hardware::tests::lazy::V1_0::implementation::Lazy();
    auto serviceRegistrar = std::make_shared<::android::hardware::LazyServiceRegistrar>();
    const ::android::status_t status = serviceRegistrar->registerService(lazy);
    if (status != ::android::OK) {
        return 1;  // or handle error
    }

    // Adds this thread to the threadpool, resulting in one total
    // thread in the threadpool. We could also do other things, but
    // would have to specify 'false' to willJoin in configureRpcThreadpool.
    ::android::hardware::joinRpcThreadpool();
    return 1;  // joinRpcThreadpool should never return
}
