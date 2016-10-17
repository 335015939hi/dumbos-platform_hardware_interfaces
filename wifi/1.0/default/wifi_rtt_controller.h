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

#ifndef WIFI_RTT_CONTROLLER_H_
#define WIFI_RTT_CONTROLLER_H_

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiIface.h>
#include <android/hardware/wifi/1.0/IWifiRttController.h>
#include <android/hardware/wifi/1.0/IWifiRttControllerEventCallback.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control all RTT operations.
 */
class WifiRttController : public IWifiRttController {
 public:
  WifiRttController(const sp<IWifiIface>& bound_iface,
                    const std::weak_ptr<WifiLegacyHal> legacy_hal);
  // Invalidate this instance once the HAL is stopped or chip mode changes.
  void invalidate();

  // HIDL methods exposed.
  Return<void> getBoundIface(getBoundIface_cb cb) override;
  Return<void> registerEventCallback(
      const sp<IWifiRttControllerEventCallback>& event_callback,
      registerEventCallback_cb hidl_status_cb) override;
  Return<void> rangeRequest(uint32_t cmdId,
                            const hidl_vec<RttConfig>& rttConfigs,
                            rangeRequest_cb hidl_status_cb) override;
  Return<void> rangeCancel(uint32_t cmdId,
                           const hidl_vec<hidl_array<uint8_t, 6 /* 6 */>>& addrs,
                           rangeCancel_cb hidl_status_cb) override;
  Return<void> setChannelMap(uint32_t cmdId,
                             const RttChannelMap& params,
                             uint32_t numDw,
                             setChannelMap_cb hidl_status_cb) override;
  Return<void> clearChannelMap(uint32_t cmdId,
                               clearChannelMap_cb hidl_status_cb) override;
  Return<void> getCapabilities(getCapabilities_cb hidl_status_cb) override;
  Return<void> setDebugCfg(RttDebugType Type,
                           setDebugCfg_cb hidl_status_cb) override;
  Return<void> getDebugInfo(getDebugInfo_cb hidl_status_cb) override;
  Return<void> setLci(uint32_t cmdId,
                      const RttLciInformation& lci,
                      setLci_cb hidl_status_cb) override;
  Return<void> setLcr(uint32_t cmdId,
                      const RttLcrInformation& lcr,
                      setLcr_cb hidl_status_cb) override;
  Return<void> getResponderInfo(getResponderInfo_cb hidl_status_cb) override;
  Return<void> enableResponder(uint32_t cmdId,
                               const WifiChannelInfo& channelHint,
                               uint32_t maxDurationSeconds,
                               const RttResponder& info,
                               enableResponder_cb hidl_status_cb) override;
  Return<void> disableResponder(uint32_t cmdId,
                                disableResponder_cb hidl_status_cb) override;

 private:
  sp<IWifiIface> bound_iface_;
  std::weak_ptr<WifiLegacyHal> legacy_hal_;
  std::vector<sp<IWifiRttControllerEventCallback>> event_callbacks_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(WifiRttController);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_RTT_CONTROLLER_H_
