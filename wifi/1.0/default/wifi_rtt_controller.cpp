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

#include "wifi_rtt_controller.h"

#include <android-base/logging.h>

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiRttController::WifiRttController(
    const sp<IWifiIface>& bound_iface,
    const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : bound_iface_(bound_iface), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiRttController::invalidate() {
  legacy_hal_.reset();
  is_valid_ = false;
}

Return<void> WifiRttController::getBoundIface(getBoundIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID),
       nullptr);
    return Void();
  }
  cb(createWifiStatus(WifiStatusCode::SUCCESS), bound_iface_);
  return Void();
}

Return<void> WifiRttController::registerEventCallback(
    const sp<IWifiRttControllerEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::rangeRequest(
    uint32_t /* cmdId */,
    const hidl_vec<RttConfig>& /* rttConfigs */,
    rangeRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::rangeCancel(
    uint32_t /* cmdId */,
    const hidl_array<uint8_t, 6 /* 6 */>& /* addrs */,
    rangeCancel_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  return Void();
}

Return<void> WifiRttController::setChannelMap(uint32_t /* cmdId */,
                                              const RttChannelMap& /* params */,
                                              uint32_t /* numDw */,
                                              setChannelMap_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::clearChannelMap(
    uint32_t /* cmdId */, clearChannelMap_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::getCapabilities(
    getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID),
        RttCapabilities());
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), RttCapabilities());
  return Void();
}

Return<void> WifiRttController::setDebugCfg(RttDebugType /* Type */,
                                            setDebugCfg_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::getDebugInfo(getDebugInfo_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID),
        RttDebugInfo());
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), RttDebugInfo());
  return Void();
}

Return<void> WifiRttController::setLci(uint32_t /* cmdId */,
                                       const RttLciInformation& /* lci */,
                                       setLci_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::setLcr(uint32_t /* cmdId */,
                                       const RttLcrInformation& /* lcr */,
                                       setLcr_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::getResponderInfo(
    getResponderInfo_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID),
        RttResponder());
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), RttResponder());
  return Void();
}

Return<void> WifiRttController::enableResponder(
    uint32_t /* cmdId */,
    const WifiChannelInfo& /* channelHint */,
    uint32_t /* maxDurationSeconds */,
    const RttResponder& /* info */,
    enableResponder_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiRttController::disableResponder(
    uint32_t /* cmdId */, disableResponder_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(
        createWifiStatus(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
