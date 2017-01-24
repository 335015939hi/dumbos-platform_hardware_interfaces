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
#include "wifi_chip.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_1 {
namespace implementation {

eturn<void> WifiChip::createNanIface(createNanIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createNanIfaceInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getNanIfaceNamesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getNanIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

Return<void> WifiChip::removeNanIface(const hidl_string& ifname,
                                      removeNanIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::removeNanIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

std::pair<WifiStatus, sp<IWifiNanIface>> WifiChip::createNanIfaceInternal() {
  // Only 1 of NAN or P2P iface can be active at a time.
  if (current_mode_id_ != kStaChipModeId || nan_iface_.get() ||
      p2p_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE), {}};
  }
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  for (const auto& callback : event_callbacks_) {
    callback->onIfaceAdded(IfaceType::NAN, ifname);
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_};
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiChip::getNanIfaceNamesInternal() {
  if (!nan_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          {legacy_hal_.lock()->getNanIfaceName()}};
}

std::pair<WifiStatus, sp<IWifiNanIface>> WifiChip::getNanIfaceInternal(
    const std::string& ifname) {
  if (!nan_iface_.get() || (ifname != legacy_hal_.lock()->getNanIfaceName())) {
    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_};
}

WifiStatus WifiChip::removeNanIfaceInternal(const std::string& ifname) {
  if (!nan_iface_.get() || (ifname != legacy_hal_.lock()->getNanIfaceName())) {
    return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
  }
  invalidateAndClear(nan_iface_);
  for (const auto& callback : event_callbacks_) {
    callback->onIfaceRemoved(IfaceType::NAN, ifname);
  }
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace hardware
}  // namespace android
