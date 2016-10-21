#include "ServiceManager.h"

#include <hidl/HidlSupport.h>

namespace android {
namespace hardware {
namespace manager {
namespace V1_0 {
namespace implementation {

struct HidlService {
      HidlService(const string &name,
                  const sp<IBinder>& service,
                  const Version& version,
                  const string &metaVersion)
      : mName(name),
        mVersion(version),
        mMetaVersion(metaVersion),
        mService(service) {}

      sp<IBinder> getService() const {
          return mService;
      }

      void setService(const sp<IBinder>& service) {
          mService = service;
      }

      hidl_version& getVersion() const {
          return mVersion;
      }

      bool supportsVersion(hidl_version version) {
          if (version.get_major() == mVersion.get_major() &&
                  version.get_minor() <= mVersion.get_minor()) {
              return true;
          }
          // TODO remove log
          ALOGE("Service doesn't support version %u.%u", version.get_major(), version.get_minor());
          return false;
      }

private:
      const std::string                     mName;
      const Version                         mVersion;
      const std::string                     mMetaVersion;
      sp<IBinder>                           mService;

};

// Methods from ::android::hardware::manager::V1_0::IServiceManager follow.
Return<void> ServiceManager::HIDL_INTERNAL_getService(const hidl_string& name, const Version& version, HIDL_INTERNAL_getService_cb _hidl_cb)  {
    return HIDL_INTERNAL_checkService(name, version);
}

Return<void> ServiceManager::HIDL_INTERNAL_checkService(const hidl_string& name, const Version& version, HIDL_INTERNAL_checkService_cb _hidl_cb)  {
    const string name_str = name.c_str();
    auto numEntries = mServiceMap.count(name_str);
    auto service_iter = mServiceMap.find(name_str);

    while (numEntries > 0) {
        if (service_iter->second->supportsVersion(version)) {
            return service_iter->second->getService();
        }         
        --numEntries;
        ++service_iter;
    }
    return nullptr;

    return Void();
}

Return<void> ServiceManager::HIDL_INTERNAL_addService(const hidl_string& name, const sp<IDummy>& service, const Version& version)  {
    const string name_str = name.c_str();
    size_t numEntries = mServiceMap.count(name_str);
    auto service_iter = mServiceMap.find(name_str);
    bool replaced = false;
    while (numEntries > 0) {
        if (service_iter->second->getVersion() == version) {
            // Just update service reference
            service_iter->second->setService(service);
            replaced = true;
            break;
        }
        --numEntries;
        ++service_iter;
    }
    if (!replaced) {
        mServiceMap.insert({name_str, unique_ptr<HidlService>(
                new HidlService(name_str, service, version, ""))});
    }

    // TODO link to death so we know when it dies

    return Void();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace manager
}  // namespace hardware
}  // namespace android
