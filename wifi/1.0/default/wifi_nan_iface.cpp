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

#include <android-base/logging.h>

#include "hidl_return_util.h"
#include "hidl_struct_util.h"
#include "wifi_nan_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiNanIface::WifiNanIface(
    const std::string& ifname,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {
}

void WifiNanIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

bool WifiNanIface::isValid() {
  return is_valid_;
}

Return<void> WifiNanIface::getName(getName_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getNameInternal,
                         hidl_status_cb);
}

Return<void> WifiNanIface::getType(getType_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::getTypeInternal,
                         hidl_status_cb);
}

Return<void> WifiNanIface::registerEventCallback(
    const sp<IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiNanIface::registerEventCallbackInternal,
                         hidl_status_cb,
                         callback);
}

std::pair<WifiStatus, std::string> WifiNanIface::getNameInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiNanIface::getTypeInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::NAN};
}

WifiStatus WifiNanIface::registerEventCallbackInternal(
    const sp<IWifiNanIfaceEventCallback>& callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed and/or
  // make sure that the same callback is only registered once (i.e. detect duplicates)
  // OR: consider having a single listener - not clear why multiple listeners (managers) are
  // necessary, nor how they would coordinate (at least command IDs).
  event_callbacks_.emplace_back(callback);
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
