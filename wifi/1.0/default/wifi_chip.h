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

#ifndef WIFI_CHIP_H_
#define WIFI_CHIP_H_

#include <map>

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiChip.h>

#include "wifi_ap_iface.h"
#include "wifi_legacy_hal.h"
#include "wifi_nan_iface.h"
#include "wifi_p2p_iface.h"
#include "wifi_rtt_controller.h"
#include "wifi_sta_iface.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control a Wifi HAL chip instance.
 * Since there is only a single chip instance used today, there is no
 * identifying handle information stored here.
 */
class WifiChip : public IWifiChip {
 public:
  WifiChip(ChipId chip_id, const std::weak_ptr<WifiLegacyHal>& legacy_hal);
  // Invalidate this instance once the HAL is stopped.
  void invalidate();

  // HIDL methods exposed.
  Return<ChipId> getId() override;
  Return<void> registerEventCallback(
      const sp<IWifiChipEventCallback>& callback) override;
  Return<void> getAvailableModes(getAvailableModes_cb cb) override;
  Return<void> configureChip(uint32_t mode_id) override;
  Return<uint32_t> getMode() override;
  Return<void> requestChipDebugInfo() override;
  Return<void> requestDriverDebugDump() override;
  Return<void> requestFirmwareDebugDump() override;
  Return<void> createApIface(createApIface_cb cb) override;
  Return<void> getApIfaceNames(getApIfaceNames_cb cb) override;
  Return<void> getApIface(const hidl_string& ifname, getApIface_cb cb) override;
  Return<void> createNanIface(createNanIface_cb cb) override;
  Return<void> getNanIfaceNames(getNanIfaceNames_cb cb) override;
  Return<void> getNanIface(const hidl_string& ifname,
                           getNanIface_cb cb) override;
  Return<void> createP2pIface(createP2pIface_cb cb) override;
  Return<void> getP2pIfaceNames(getP2pIfaceNames_cb cb) override;
  Return<void> getP2pIface(const hidl_string& ifname,
                           getP2pIface_cb cb) override;
  Return<void> createStaIface(createStaIface_cb cb) override;
  Return<void> getStaIfaceNames(getStaIfaceNames_cb cb) override;
  Return<void> getStaIface(const hidl_string& ifname,
                           getStaIface_cb cb) override;
  Return<void> createRttController(const sp<IWifiIface>& bound_iface,
                                   createRttController_cb cb) override;

 private:
  void invalidateAndRemoveAllIfaces();
  void callWithEachCallback(
      const std::function<Return<void>(sp<IWifiChipEventCallback>)>& method);

  ChipId chip_id_;
  std::weak_ptr<WifiLegacyHal> legacy_hal_;
  std::vector<sp<IWifiChipEventCallback>> callbacks_;
  std::map<std::string, sp<WifiApIface>> ap_ifaces_;
  std::map<std::string, sp<WifiNanIface>> nan_ifaces_;
  std::map<std::string, sp<WifiP2pIface>> p2p_ifaces_;
  std::map<std::string, sp<WifiStaIface>> sta_ifaces_;
  std::vector<sp<WifiRttController>> rtt_controllers_;

  DISALLOW_COPY_AND_ASSIGN(WifiChip);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_CHIP_H_
