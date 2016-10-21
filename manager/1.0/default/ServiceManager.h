#ifndef HIDL_GENERATED_android_hardware_manager_V1_0_ServiceManager_H_
#define HIDL_GENERATED_android_hardware_manager_V1_0_ServiceManager_H_

#include <android/hardware/manager/1.0/IServiceManager.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <map>

namespace android {
namespace hardware {
namespace manager {
namespace V1_0 {
namespace implementation {

using ::android::hardware::manager::V1_0::IDummy;
using ::android::hardware::manager::V1_0::IServiceManager;
using ::android::hardware::manager::V1_0::Version;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct HidlService;

struct ServiceManager : public IServiceManager {

    // Methods from ::android::hardware::manager::V1_0::IServiceManager follow.
    Return<void> HIDL_INTERNAL_getService(const hidl_string& name, const Version& version, HIDL_INTERNAL_getService_cb _hidl_cb)  override;
    Return<void> HIDL_INTERNAL_checkService(const hidl_string& name, const Version& version, HIDL_INTERNAL_checkService_cb _hidl_cb)  override;
    Return<void> HIDL_INTERNAL_addService(const hidl_string& name, const sp<IDummy>& service, const Version& version)  override;

private:

    // Access to this map doesn't need to be locked, since hwservicemanager
    // is single-threaded.
    std::multimap<string, std::unique_ptr<HidlService>> mServiceMap;

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace manager
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_manager_V1_0_ServiceManager_H_
