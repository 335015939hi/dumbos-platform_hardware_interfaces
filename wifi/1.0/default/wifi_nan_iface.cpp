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

#include "wifi_nan_iface.h"

#include <android-base/logging.h>

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiNanIface::WifiNanIface(const std::string& ifname,
                           const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiNanIface::invalidate() {
  legacy_hal_.reset();
  is_valid_ = false;
}

Return<void> WifiNanIface::getName(getName_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   hidl_string());
    return Void();
  }
  hidl_string hidl_ifname;
  hidl_ifname.setToExternal(ifname_.c_str(), ifname_.size());
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifname);
  return Void();
}

Return<void> WifiNanIface::getType(getType_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   IfaceType::NAN);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::NAN);
  return Void();
}

Return<void> WifiNanIface::registerEventCallback(
    const sp<IWifiNanIfaceEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::enableRequest(uint32_t /* cmdId */,
                                         const NanEnableRequest& /* msg */,
                                         enableRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::disableRequest(uint32_t /* cmdId */,
                                          disableRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::publishRequest(uint32_t /* cmdId */,
                                          const NanPublishRequest& /* msg */,
                                          publishRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::publishCancelRequest(
    uint32_t /* cmdId */,
    const NanPublishCancelRequest& /* msg */,
    publishCancelRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::subscribeRequest(
    uint32_t /* cmdId */,
    const NanSubscribeRequest& /* msg */,
    subscribeRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::subscribeCancelRequest(
    uint32_t /* cmdId */,
    const NanSubscribeCancelRequest& /* msg */,
    subscribeCancelRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::transmitFollowupRequest(
    uint32_t /* cmdId */,
    const NanTransmitFollowupRequest& /* msg */,
    transmitFollowupRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::configRequest(uint32_t /* cmdId */,
                                         const NanConfigRequest& /* msg */,
                                         configRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::beaconSdfPayloadRequest(
    uint32_t /* cmdId */,
    const NanBeaconSdfPayloadRequest& /* msg */,
    beaconSdfPayloadRequest_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::getVersion(getVersion_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   0);
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), 0);
  return Void();
}

Return<void> WifiNanIface::getCapabilities(uint32_t /* cmdId */,
                                           getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::dataInterfaceCreate(
    uint32_t /* cmdId */,
    const hidl_string& /* ifaceName */,
    dataInterfaceCreate_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::dataInterfaceDelete(
    uint32_t /* cmdId */,
    const hidl_string& /* ifaceName */,
    dataInterfaceDelete_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}
Return<void> WifiNanIface::dataRequestInitiator(
    uint32_t /* cmdId */,
    const NanDataPathInitiatorRequest& /* msg */,
    dataRequestInitiator_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::dataIndicationResponse(
    uint32_t /* cmdId */,
    const NanDataPathIndicationResponse& /* msg */,
    dataIndicationResponse_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiNanIface::dataEnd(uint32_t /* cmdId */,
                                   const NanDataPathEndRequest& /* msg */,
                                   dataEnd_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
