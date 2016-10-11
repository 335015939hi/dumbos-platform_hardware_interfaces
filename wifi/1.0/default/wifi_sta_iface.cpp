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

#include "wifi_sta_iface.h"

#include <android-base/logging.h>

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiStaIface::WifiStaIface(const std::string& ifname,
                           const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiStaIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiStaIface::getName(getName_cb hidl_status_cb) {
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

Return<void> WifiStaIface::getType(getType_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   IfaceType::STA);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::STA);
  return Void();
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& event_callback,
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

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
