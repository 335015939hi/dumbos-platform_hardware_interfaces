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

#include "failure_reason_util.h"

namespace {
using android::hardware::hidl_vec;
using android::hardware::hidl_string;

template <typename Iface>
hidl_vec<hidl_string> getIfaceNames(
    const std::map<std::string, android::sp<Iface>>& ifaces) {
  std::vector<hidl_string> ifnames;
  for (const auto& it : ifaces) {
    hidl_string hidl_ifname;
    hidl_ifname.setToExternal(it.first.c_str(), it.first.size());
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  return hidl_ifnames;
}

template <typename Iface>
android::sp<Iface> getIface(
    const hidl_string& ifname,
    const std::map<std::string, android::sp<Iface>>& ifaces) {
  const auto it = ifaces.find(ifname.c_str());
  if (it == ifaces.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

template <typename Iface>
void invalidateAndRemoveIfaces(
    std::map<std::string, android::sp<Iface>>& ifaces) {
  for (const auto& it : ifaces) {
    it.second->invalidate();
  }
  ifaces.clear();
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(ChipId chip_id,
                   const std::weak_ptr<WifiLegacyHal>& legacy_hal)
    : chip_id_(chip_id), legacy_hal_(legacy_hal) {}

void WifiChip::invalidate() {
  invalidateAndRemoveAllIfaces();
  legacy_hal_.reset();
  callbacks_.clear();
}

Return<ChipId> WifiChip::getId() {
  return chip_id_;
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& callback) {
  if (!legacy_hal_.lock())
    return Void();
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.emplace_back(callback);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<ChipMode>());
    return Void();
  } else {
    // TODO add implementation
    return Void();
  }
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/) {
  if (!legacy_hal_.lock())
    return Void();

  invalidateAndRemoveAllIfaces();
  // TODO add implementation
  return Void();
}

Return<uint32_t> WifiChip::getMode() {
  if (!legacy_hal_.lock())
    return 0;
  // TODO add implementation
  return 0;
}

Return<void> WifiChip::requestChipDebugInfo() {
  if (!legacy_hal_.lock())
    return Void();

  IWifiChipEventCallback::ChipDebugInfo result;

  std::pair<wifi_error, std::string> ret =
      legacy_hal_.lock()->getDriverVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.driverDescription = ret.second.c_str();

  ret = legacy_hal_.lock()->getFirmwareVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.firmwareDescription = ret.second.c_str();

  for (const auto& callback : callbacks_) {
    callback->onChipDebugInfoAvailable(result);
  }
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump() {
  if (!legacy_hal_.lock())
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& driver_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onDriverDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump() {
  if (!legacy_hal_.lock())
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& firmware_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onFirmwareDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::createApIface(createApIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  sp<WifiApIface> iface = new WifiApIface(ifname, legacy_hal_);
  ap_ifaces_.emplace(ifname, iface);
  cb(iface);
  return Void();
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  cb(getIfaceNames(ap_ifaces_));
  return Void();
}

Return<void> WifiChip::getApIface(const hidl_string& ifname, getApIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  cb(getIface(ifname, ap_ifaces_));
  return Void();
}

Return<void> WifiChip::createNanIface(createNanIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  sp<WifiNanIface> iface = new WifiNanIface(ifname, legacy_hal_);
  nan_ifaces_.emplace(ifname, iface);
  cb(iface);
  return Void();
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  cb(getIfaceNames(nan_ifaces_));
  return Void();
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  cb(getIface(ifname, nan_ifaces_));
  return Void();
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  sp<WifiP2pIface> iface = new WifiP2pIface(ifname, legacy_hal_);
  p2p_ifaces_.emplace(ifname, iface);
  cb(iface);
  return Void();
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  cb(getIfaceNames(p2p_ifaces_));
  return Void();
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  cb(getIface(ifname, p2p_ifaces_));
  return Void();
}

Return<void> WifiChip::createStaIface(createStaIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sp<WifiStaIface> iface = new WifiStaIface(ifname, legacy_hal_);
  sta_ifaces_.emplace(ifname, iface);
  cb(iface);
  return Void();
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  cb(getIfaceNames(sta_ifaces_));
  return Void();
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(nullptr);
    return Void();
  }

  cb(getIface(ifname, sta_ifaces_));
  return Void();
}

void WifiChip::invalidateAndRemoveAllIfaces() {
  invalidateAndRemoveIfaces(ap_ifaces_);
  invalidateAndRemoveIfaces(nan_ifaces_);
  invalidateAndRemoveIfaces(p2p_ifaces_);
  invalidateAndRemoveIfaces(sta_ifaces_);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
