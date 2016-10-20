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

namespace {
using android::sp;
using android::hardware::hidl_vec;
using android::hardware::hidl_string;

hidl_vec<hidl_string> createHidlVecOfIfaceNames(const std::string& ifname) {
  std::vector<hidl_string> ifnames;
  if (!ifname.empty()) {
    hidl_string hidl_ifname;
    hidl_ifname = ifname.c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  return hidl_ifnames;
}

template <typename Iface>
void invalidateAndClear(sp<Iface>& iface) {
  if (iface.get()) {
    iface->invalidate();
    iface.clear();
  }
}
}  // namepsace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(ChipId chip_id,
                   const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : chip_id_(chip_id), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiChip::invalidate() {
  invalidateAndRemoveAllIfaces();
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiChip::getId(getId_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   UINT32_MAX);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), chip_id_);
  return Void();
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::getCapabilities(getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   0);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), 0);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<ChipMode>());
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 hidl_vec<ChipMode>());
  return Void();
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/,
                                     configureChip_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }

  invalidateAndRemoveAllIfaces();
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::getMode(getMode_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   UINT32_MAX);
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), 0);
  return Void();
}

Return<void> WifiChip::requestChipDebugInfo(
    requestChipDebugInfo_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   IWifiChip::ChipDebugInfo());
    return Void();
  }

  IWifiChip::ChipDebugInfo result;

  std::pair<legacy_hal::wifi_error, std::string> legacy_ret =
      legacy_hal_.lock()->getDriverVersion();
  if (legacy_ret.first != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << legacyErrorToString(legacy_ret.first);
    WifiStatus status = createWifiStatusFromLegacyError(
        legacy_ret.first, "failed to get driver version");
    hidl_status_cb(status, result);
    return Void();
  }
  result.driverDescription = legacy_ret.second.c_str();

  legacy_ret = legacy_hal_.lock()->getFirmwareVersion();
  if (legacy_ret.first != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << legacyErrorToString(legacy_ret.first);
    WifiStatus status = createWifiStatusFromLegacyError(
        legacy_ret.first, "failed to get firmware version");
    hidl_status_cb(status, result);
    return Void();
  }
  result.firmwareDescription = legacy_ret.second.c_str();

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), result);
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump(
    requestDriverDebugDump_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<uint8_t>());
    return Void();
  }

  std::pair<legacy_hal::wifi_error, std::vector<char>> legacy_ret =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (legacy_ret.first != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << legacyErrorToString(legacy_ret.first);
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_ret.first),
                   hidl_vec<uint8_t>());
    return Void();
  }

  auto& driver_dump = legacy_ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_data);
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump(
    requestFirmwareDebugDump_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<uint8_t>());
    return Void();
  }

  std::pair<legacy_hal::wifi_error, std::vector<char>> legacy_ret =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (legacy_ret.first != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << legacyErrorToString(legacy_ret.first);
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_ret.first),
                   hidl_vec<uint8_t>());
    return Void();
  }

  auto& firmware_dump = legacy_ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_data);
  return Void();
}

Return<void> WifiChip::createApIface(createApIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  ap_iface_ = new WifiApIface(ifname, legacy_hal_);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_);
  return Void();
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (ap_iface_.get()) {
    ifname = legacy_hal_.lock()->getApIfaceName().c_str();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getApIface(const hidl_string& ifname,
                                  getApIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  if (ap_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getApIfaceName())) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_);
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS),
                   nullptr);
  }
  return Void();
}

Return<void> WifiChip::createNanIface(createNanIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_);
  return Void();
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (nan_iface_.get()) {
    ifname = legacy_hal_.lock()->getNanIfaceName().c_str();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  if (nan_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getNanIfaceName())) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_);
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS),
                   nullptr);
  }
  return Void();
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  p2p_iface_ = new WifiP2pIface(ifname, legacy_hal_);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_);
  return Void();
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (p2p_iface_.get()) {
    ifname = legacy_hal_.lock()->getP2pIfaceName().c_str();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  if (p2p_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getP2pIfaceName())) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_);
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS),
                   nullptr);
  }
  return Void();
}

Return<void> WifiChip::createStaIface(createStaIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sta_iface_ = new WifiStaIface(ifname, legacy_hal_);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_);
  return Void();
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (sta_iface_.get()) {
    ifname = legacy_hal_.lock()->getStaIfaceName().c_str();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  if (sta_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getStaIfaceName())) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_);
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS),
                   nullptr);
  }
  return Void();
}

Return<void> WifiChip::createRttController(
    const sp<IWifiIface>& bound_iface, createRttController_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   nullptr);
    return Void();
  }

  sp<WifiRttController> rtt = new WifiRttController(bound_iface, legacy_hal_);
  rtt_controllers_.emplace_back(rtt);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), rtt);
  return Void();
}

Return<void> WifiChip::getDebugRingBuffersStatus(
    getDebugRingBuffersStatus_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   hidl_vec<WifiDebugRingBufferStatus>());
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 hidl_vec<WifiDebugRingBufferStatus>());
  return Void();
}

Return<void> WifiChip::startLoggingToDebugRingBuffer(
    const hidl_string& /* ringName */,
    WifiDebugRingBufferVerboseLevel /* verboseLevel */,
    uint32_t /* maxIntervalInSec */,
    uint32_t /* minDataSizeInBytes */,
    startLoggingToDebugRingBuffer_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::forceDumpToDebugRingBuffer(
    const hidl_string& /* ringName */,
    forceDumpToDebugRingBuffer_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID));
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiChip::getDebugHostWakeReasonStats(
    getDebugHostWakeReasonStats_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_CHIP_INVALID),
                   WifiDebugHostWakeReasonStats());
    return Void();
  }
  // TODO: add implementation
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS),
                 WifiDebugHostWakeReasonStats());
  return Void();
}

void WifiChip::invalidateAndRemoveAllIfaces() {
  invalidateAndClear(ap_iface_);
  invalidateAndClear(nan_iface_);
  invalidateAndClear(p2p_iface_);
  invalidateAndClear(sta_iface_);
  // Since all the ifaces are invalid now, all RTT controller objects
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
