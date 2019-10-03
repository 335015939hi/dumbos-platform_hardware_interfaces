# Implementing Health 2.1 HAL

1. Install common binderized service. The binderized service `dlopen()`s
   passthrough implementations on the device, so there is no need to write
   your own.

    ```mk
    # Install default binderized implementation to vendor.
    PRODUCT_PACKAGES += android.hardware.health@2.1-service
    ```

1. Delete existing VINTF manifest entry. Search for `android.hardware.health` in
   your device manifest, and delete the whole `<hal>` entry for older versions
   of the HAL. Instead, when `android.hardware.health@2.1-service` is installed,
   a VINTF manifest fragment is installed to `/vendor/etc/vintf`, so there is
   no need to manually specify it in your device manifest. See
   [Manifest fragments](https://source.android.com/devices/architecture/vintf/objects#manifest-fragments)
   for details.

1. Install the proper passthrough implemetation.

    1. If you want to use default implementation:

        ```mk
        # Install default passthrough implementation to vendor.
        PRODUCT_PACKAGES += android.hardware.health@2.1-impl
        ```

        You are done. Otherwise, go to the next step.

    1. If one of the following applies, you may want to write an implementation:

        * You have a board or device specific `libhealthd`.
        * You are upgrading from 1.0 health HAL.
        * You are upgrading from a customized 2.0 health HAL implementation.

# Writing your own implementation

Below is sample code for the implementation.

## `device/<manufacturer>/<device>/health/Android.bp`

```bp
cc_library_shared {
    name: "android.hardware.health@2.1-impl-<device>",
    stem: "android.hardware.health@2.0-impl-2.1-<device>",

    // Install to vendor and recovery.
    proprietary: true,
    recovery_available: true,

    relative_install_path: "hw",

    shared_libs: [
        "libbase",
        "libcutils",
        "libhidlbase",
        "liblog",
        "libutils",
        "android.hardware.health@2.1",
        "android.hardware.health@2.0",
    ],

    static_libs: [
        "android.hardware.health@1.0-convert",
        "libbatterymonitor",
        "libhealthloop",
        "libhealth2impl",
        // "libhealthd.<device>"
    ],

    srcs: [
        "MyHealth.cpp",
        "impl.cpp",
    ],

    // No vintf_fragments because both -impl and -service should have been
    // installed.
}
```

## `device/<manufacturer>/<device>/health/impl.cpp`

```c++
#include <memory>
#include <string_view>

#include <health/utils.h>
#include "MyHealth.h"

using ::android::sp;
using ::android::hardware::health::InitHealthdConfig;
using ::android::hardware::health::V2_1::IHealth;
using ::android::hardware::health::V2_1::implementation::MyHealth;

using namespace std::literals;
extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    if (instance != "default"sv) {
        return nullptr;
    }
    auto config = std::make_unique<healthd_config>();
    InitHealthdConfig(config.get());

    // healthd_board_init(config.get());

    return new MyHealth(std::move(config));
}

```

## `device/<manufacturer>/<device>/health/MyHealth.h`

```c++
#pragma once

#include <memory>

#include <health2impl/Health.h>
#include <hidl/Status.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// android::hardware::health::V2_1::implementation::Health implements most
// defaults. Uncomment functions that you need to override.
class MyHealth : public Health {
  public:
    MyHealth(std::unique_ptr<healthd_config>&& config)
        : Health(std::move(config)) {}

    // A subclass can override this if these information should be retrieved
    // differently.
    // Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override;
    // Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override;
    // Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override;
    // Return<void> getCapacity(getCapacity_cb _hidl_cb) override;
    // Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override;
    // Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override;
    // Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override;
    // Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override;
    // Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override;

    // Functions introduced in Health HAL 2.1.
    // Return<void> getHealthConfig(getHealthConfig_cb _hidl_cb) override;
    // Return<void> getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) override;
    // Return<void> getScreenOn(getScreenOn_cb _hidl_cb) override;

  protected:
    // A subclass can override this to modify any health info object before
    // returning to clients. This is similar to healthd_board_battery_update().
    // By default, it does nothing.
    // virtual void UpdateHealthInfo(HealthInfo* health_info);
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

```

# Upgrading with a customized libhealthd or from Health HAL 1.0

`libhealthd` contains two functions: `healthd_board_init()` and
`healthd_board_battery_update()`. Similarly, Health HAL 1.0 contains `init()`
and `update()`, with an additional `energyCounter()` function.

* `healthd_board_init()` / `@1.0::IHealth.init()` should be called before
  passing the `healthd_config` struct to your `MyHealth` class. See
  `HIDL_FETCH_IHealth` in `impl.cpp` above.
* `healthd_board_battery_update()` / `@1.0::IHealth.update()` should be called
  in `MyHealth::UpdateHealthInfo()`. Example:

  ```c++
  void MyHealth::UpdateHealthInfo(HealthInfo* health_info) {
      struct BatteryProperties props;
      convertFromHealthInfo(health_info->legacy.legacy, &props);
      healthd_board_battery_update(&props);
      convertToHealthInfo(&props, health_info->legacy.legacy);
  }
  ```
  For efficiency, you should move code in `healthd_board_battery_update` to
  `MyHealth::UpdateHealthInfo` and modify `health_info` directly to avoid
  conversion to `BatteryProperties`.

* Code for `@1.0::IHealth.energyCounter()` should be moved to
  `MyHealth::getEnergyCounter()`. Example:

  ```c++
  Return<void> Health::getEnergyCounter(getEnergyCounter_cb _hidl_cb) {
      int64_t energy = /* ... */;
      _hidl_cb(Result::SUCCESS, energy);
      return Void();
  }

  ```

# Upgrading from Health HAL 2.0

* If you have implemented `healthd_board_init()` and/or
  `healthd_board_battery_update()` (instead of using `libhealthd.default`),
  see [the section above](#upgrading-with-a-customized-libhealthd-or-from-health-hal-1_0)
  for instructions to convert them.

* If you have implemented `get_storage_info()` and/or `get_disk_stats()`
  (instead of using libhealthstoragedefault), implement `MyHealth::getDiskStats`
  and/or `MyHealth::getStorageInfo` directly. There is no need to override
  `MyHealth::getHealthInfo` or `MyHealth::getHealthInfo_2_1` because they call
  `getDiskStats` and `getStorageInfo` to retrieve storage information.

# Update necessary SELinux permissions

```
# device/<manufacturer>/<device>/sepolicy/vendor/file_contexts
/vendor/lib(64)?/hw/android\.hardware\.health@2\.1-impl u:object_r:same_process_hal_file:s0
# device/<manufacturer>/<device>/sepolicy/vendor/hal_health_default.te
# Add device specific permissions to hal_health_default domain, especially
# if a device-specific libhealthd is used and/or device-specific storage related
# APIs are implemented.
```
