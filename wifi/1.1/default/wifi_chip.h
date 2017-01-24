/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WIFI_CHIP_1_1_H_
#define WIFI_CHIP_1_1_H_

#include <map>

#include <android-base/macros.h>
#include <android/hardware/wifi/1.1/IWifiChip.h>
#include "../../1.0/default/wifi_chip.h"

#include "wifi_nan_iface.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_1 {
namespace implementation {

/**
 * HIDL interface object used to control a Wifi HAL chip instance.
 * Since there is only a single chip instance used today, there is no
 * identifying handle information stored here.
 */
class WifiChip : public android::hardware::wifi::V1_0::implementation::WifiChip {
 public:
  // New HIDL methods exposed.
  Return<void> createNanIface(createNanIface_cb hidl_status_cb) override;
  Return<void> getNanIfaceNames(getNanIfaceNames_cb hidl_status_cb) override;
  Return<void> getNanIface(const hidl_string& ifname,
                           getNanIface_cb hidl_status_cb) override;
  Return<void> removeNanIface(const hidl_string& ifname,
                              removeNanIface_cb hidl_status_cb) override;

 private:
  // Corresponding worker functions for the HIDL methods.
  std::pair<WifiStatus, sp<IWifiNanIface>> createNanIfaceInternal();
  std::pair<WifiStatus, std::vector<hidl_string>> getNanIfaceNamesInternal();
  std::pair<WifiStatus, sp<IWifiNanIface>> getNanIfaceInternal(
      const std::string& ifname);
  WifiStatus removeNanIfaceInternal(const std::string& ifname);

  sp<WifiNanIface> nan_iface_;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_CHIP_1_1_H_
