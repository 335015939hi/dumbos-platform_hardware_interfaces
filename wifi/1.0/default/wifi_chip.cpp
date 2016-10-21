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

#include "hidl_return_macros.h"

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
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, UINT32_MAX);
  }
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, chip_id_);
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID);
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  HIDL_RETURN0_WITH_CB(WifiStatusCode::SUCCESS);
}

Return<void> WifiChip::getCapabilities(getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, 0);
  }
  // TODO: add implementation
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, 0);
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<ChipMode>());
  }
  // TODO: add implementation
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, hidl_vec<ChipMode>());
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/,
                                     configureChip_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID);
  }

  invalidateAndRemoveAllIfaces();
  // TODO: add implementation
  HIDL_RETURN0_WITH_CB(WifiStatusCode::SUCCESS);
}

Return<void> WifiChip::getMode(getMode_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, UINT32_MAX);
  }
  // TODO: add implementation
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, 0);
}

Return<void> WifiChip::requestChipDebugInfo(
    requestChipDebugInfo_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         IWifiChip::ChipDebugInfo());
  }

  IWifiChip::ChipDebugInfo result;

  legacy_hal::wifi_error legacy_status;
  std::string driver_desc;
  std::tie(legacy_status, driver_desc) = legacy_hal_.lock()->getDriverVersion();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << legacyErrorToString(legacy_status);
    HIDL_RETURN1_WITH_CB_FROM_LEGACY_ERROR(
        legacy_status, "failed to get driver version", result);
  }
  result.driverDescription = driver_desc.c_str();

  std::string firmware_desc;
  std::tie(legacy_status, firmware_desc) =
      legacy_hal_.lock()->getFirmwareVersion();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << legacyErrorToString(legacy_status);
    HIDL_RETURN1_WITH_CB_FROM_LEGACY_ERROR(
        legacy_status, "failed to get fimware version", result);
  }
  result.firmwareDescription = firmware_desc.c_str();

  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, result);
}

Return<void> WifiChip::requestDriverDebugDump(
    requestDriverDebugDump_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<uint8_t>());
  }

  legacy_hal::wifi_error legacy_status;
  std::vector<char> driver_dump;
  std::tie(legacy_status, driver_dump) =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << legacyErrorToString(legacy_status);
    HIDL_RETURN1_WITH_CB_FROM_LEGACY_ERROR(legacy_status, hidl_vec<uint8_t>());
  }

  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, hidl_data);
}

Return<void> WifiChip::requestFirmwareDebugDump(
    requestFirmwareDebugDump_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<uint8_t>());
  }

  legacy_hal::wifi_error legacy_status;
  std::vector<char> firmware_dump;
  std::tie(legacy_status, firmware_dump) =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << legacyErrorToString(legacy_status);
    HIDL_RETURN1_WITH_CB_FROM_LEGACY_ERROR(legacy_status, hidl_vec<uint8_t>());
  }

  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, hidl_data);
}

Return<void> WifiChip::createApIface(createApIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  ap_iface_ = new WifiApIface(ifname, legacy_hal_);
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, ap_iface_)
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<hidl_string>());
  }

  std::string ifname;
  if (ap_iface_.get()) {
    ifname = legacy_hal_.lock()->getApIfaceName().c_str();
  }
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS,
                       createHidlVecOfIfaceNames(ifname));
}

Return<void> WifiChip::getApIface(const hidl_string& ifname,
                                  getApIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  if (ap_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getApIfaceName())) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, ap_iface_);
  } else {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_INVALID_ARGS, nullptr);
  }
}

Return<void> WifiChip::createNanIface(createNanIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, nan_iface_);
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<hidl_string>());
  }

  std::string ifname;
  if (nan_iface_.get()) {
    ifname = legacy_hal_.lock()->getNanIfaceName().c_str();
  }
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS,
                       createHidlVecOfIfaceNames(ifname));
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  if (nan_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getNanIfaceName())) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, nan_iface_);
  } else {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_INVALID_ARGS, nullptr);
  }
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  p2p_iface_ = new WifiP2pIface(ifname, legacy_hal_);
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, p2p_iface_);
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<hidl_string>());
  }

  std::string ifname;
  if (p2p_iface_.get()) {
    ifname = legacy_hal_.lock()->getP2pIfaceName().c_str();
  }
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS,
                       createHidlVecOfIfaceNames(ifname));
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  if (p2p_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getP2pIfaceName())) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, p2p_iface_);
  } else {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_INVALID_ARGS, nullptr);
  }
}

Return<void> WifiChip::createStaIface(createStaIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sta_iface_ = new WifiStaIface(ifname, legacy_hal_);
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, sta_iface_);
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<hidl_string>());
  }

  std::string ifname;
  if (sta_iface_.get()) {
    ifname = legacy_hal_.lock()->getStaIfaceName().c_str();
  }
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS,
                       createHidlVecOfIfaceNames(ifname));
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  if (sta_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getStaIfaceName())) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, sta_iface_);
  } else {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_INVALID_ARGS, nullptr);
  }
}

Return<void> WifiChip::createRttController(
    const sp<IWifiIface>& bound_iface, createRttController_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID, nullptr);
  }

  sp<WifiRttController> rtt = new WifiRttController(bound_iface, legacy_hal_);
  rtt_controllers_.emplace_back(rtt);
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, rtt);
}

Return<void> WifiChip::getDebugRingBuffersStatus(
    getDebugRingBuffersStatus_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         hidl_vec<WifiDebugRingBufferStatus>());
  }
  // TODO: add implementation
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS,
                       hidl_vec<WifiDebugRingBufferStatus>());
}

Return<void> WifiChip::startLoggingToDebugRingBuffer(
    const hidl_string& /* ringName */,
    WifiDebugRingBufferVerboseLevel /* verboseLevel */,
    uint32_t /* maxIntervalInSec */,
    uint32_t /* minDataSizeInBytes */,
    startLoggingToDebugRingBuffer_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID);
  }
  // TODO: add implementation
  HIDL_RETURN0_WITH_CB(WifiStatusCode::SUCCESS);
}

Return<void> WifiChip::forceDumpToDebugRingBuffer(
    const hidl_string& /* ringName */,
    forceDumpToDebugRingBuffer_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID);
  }
  // TODO: add implementation
  HIDL_RETURN0_WITH_CB(WifiStatusCode::SUCCESS);
}

Return<void> WifiChip::getDebugHostWakeReasonStats(
    getDebugHostWakeReasonStats_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1_WITH_CB(WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         WifiDebugHostWakeReasonStats());
  }
  // TODO: add implementation
  HIDL_RETURN1_WITH_CB(WifiStatusCode::SUCCESS, WifiDebugHostWakeReasonStats());
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
