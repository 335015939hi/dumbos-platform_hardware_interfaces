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

#include "wifi_chip.h"

#include <android-base/logging.h>

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(ChipId chip_id,
                   const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : chip_id_(chip_id), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiChip::invalidate() {
  invalidateAndRemoveAllIfaces();
  legacy_hal_.reset();
  callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiChip::getId(getId_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), UINT32_MAX);
    return Void();
  }
  cb(createWifiStatus(WifiStatusCode::SUCCESS), chip_id_);
  return Void();
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& callback, registerEventCallback_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.emplace_back(callback);
  cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<ChipMode>());
    return Void();
  }
  // TODO add implementation
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_vec<ChipMode>());
  return Void();
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/,
                                     configureChip_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }

  invalidateAndRemoveAllIfaces();
  // TODO add implementation
  cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::getMode(getMode_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), UINT32_MAX);
    return Void();
  }
  // TODO add implementation
  cb(createWifiStatus(WifiStatusCode::SUCCESS), 0);
  return Void();
}

Return<void> WifiChip::requestChipDebugInfo(requestChipDebugInfo_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       IWifiChip::ChipDebugInfo());
    return Void();
  }

  IWifiChip::ChipDebugInfo result;

  std::pair<wifi_error, std::string> ret =
      legacy_hal_.lock()->getDriverVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << legacyErrorToString(ret.first);
    WifiStatus status = createWifiStatusFromLegacyError(
        ret.first, " failed to get driver version");
    cb(status, result);
    return Void();
  }
  result.driverDescription = ret.second.c_str();

  ret = legacy_hal_.lock()->getFirmwareVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << legacyErrorToString(ret.first);
    WifiStatus status = createWifiStatusFromLegacyError(
        ret.first, " failed to get firmware version");
    cb(status, result);
    return Void();
  }
  result.firmwareDescription = ret.second.c_str();

  cb(createWifiStatus(WifiStatusCode::SUCCESS), result);
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump(requestDriverDebugDump_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<uint8_t>());
    return Void();
  }

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << legacyErrorToString(ret.first);
    cb(createWifiStatusFromLegacyError(ret.first), hidl_vec<uint8_t>());
    return Void();
  }

  auto& driver_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_data);
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump(
    requestFirmwareDebugDump_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<uint8_t>());
    return Void();
  }

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << legacyErrorToString(ret.first);
    cb(createWifiStatusFromLegacyError(ret.first), hidl_vec<uint8_t>());
    return Void();
  }

  auto& firmware_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_data);
  return Void();
}

Return<void> WifiChip::createApIface(createApIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  ap_iface_ = new WifiApIface(ifname, legacy_hal_);
  cb(createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_);
  return Void();
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<hidl_string>());
    return Void();
  }

  std::vector<hidl_string> ifnames;
  if (ap_iface_.get()) {
    hidl_string hidl_ifname;
    hidl_ifname = legacy_hal_.lock()->getApIfaceName().c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifnames);
  return Void();
}

Return<void> WifiChip::getApIface(const hidl_string& ifname, getApIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  if (ap_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getApIfaceName())) {
    cb(createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_);
  } else {
    cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr);
  }
  return Void();
}

Return<void> WifiChip::createNanIface(createNanIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  cb(createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_);
  return Void();
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<hidl_string>());
    return Void();
  }

  std::vector<hidl_string> ifnames;
  if (nan_iface_.get()) {
    hidl_string hidl_ifname;
    hidl_ifname = legacy_hal_.lock()->getNanIfaceName().c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifnames);
  return Void();
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  if (nan_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getNanIfaceName())) {
    cb(createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_);
  } else {
    cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr);
  }
  return Void();
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  p2p_iface_ = new WifiP2pIface(ifname, legacy_hal_);
  cb(createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_);
  return Void();
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<hidl_string>());
    return Void();
  }

  std::vector<hidl_string> ifnames;
  if (p2p_iface_.get()) {
    hidl_string hidl_ifname;
    hidl_ifname = legacy_hal_.lock()->getP2pIfaceName().c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifnames);
  return Void();
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  if (p2p_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getP2pIfaceName())) {
    cb(createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_);
  } else {
    cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr);
  }
  return Void();
}

Return<void> WifiChip::createStaIface(createStaIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sta_iface_ = new WifiStaIface(ifname, legacy_hal_);
  cb(createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_);
  return Void();
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
       hidl_vec<hidl_string>());
    return Void();
  }

  std::vector<hidl_string> ifnames;
  if (sta_iface_.get()) {
    hidl_string hidl_ifname;
    hidl_ifname = legacy_hal_.lock()->getStaIfaceName().c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifnames);
  return Void();
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  if (sta_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getStaIfaceName())) {
    cb(createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_);
  } else {
    cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr);
  }
  return Void();
}

Return<void> WifiChip::createRttController(const sp<IWifiIface>& bound_iface,
                                           createRttController_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID), nullptr);
    return Void();
  }

  sp<WifiRttController> rtt = new WifiRttController(bound_iface, legacy_hal_);
  rtt_controllers_.emplace_back(rtt);
  cb(createWifiStatus(WifiStatusCode::SUCCESS), rtt);
  return Void();
}

void WifiChip::invalidateAndRemoveAllIfaces() {
  if (ap_iface_.get()) {
    ap_iface_->invalidate();
    ap_iface_.clear();
  }
  if (nan_iface_.get()) {
    nan_iface_->invalidate();
    nan_iface_.clear();
  }
  if (p2p_iface_.get()) {
    p2p_iface_->invalidate();
    p2p_iface_.clear();
  }
  if (sta_iface_.get()) {
    sta_iface_->invalidate();
    sta_iface_.clear();
  }
  // Since all the ifaces are invalid now, all rtt controller objects
  // using those ifaces also need to be invalidated.
  for (const auto& rtt : rtt_controllers_) {
    rtt->invalidate();
  }
  rtt_controllers_.clear();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
