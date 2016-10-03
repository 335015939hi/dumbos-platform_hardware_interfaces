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

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(std::shared_ptr<WifiLegacyHal> legacy_hal)
    : legacy_hal_(legacy_hal) {}

void WifiChip::invalidate() {
  legacy_hal_.reset();
  callbacks_.clear();
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& callback) {
  if (!legacy_hal_.get())
    return Void();
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.insert(callback);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!legacy_hal_.get()) {
    cb(hidl_vec<ChipMode>());
    return Void();
  } else {
    // TODO add implementation
    return Void();
  }
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/) {
  // TODO add implementation
  return Void();
}

Return<uint32_t> WifiChip::getMode() {
  if (!legacy_hal_.get())
    return 0;
  // TODO add implementation
  return 0;
}

Return<void> WifiChip::requestChipDebugInfo() {
  if (!legacy_hal_.get())
    return Void();

  IWifiChipEventCallback::ChipDebugInfo result;
  result.driverDescription = "<unknown>";
  result.firmwareDescription = "<unknown>";

  char buffer[256];
  bzero(buffer, sizeof(buffer));
  wifi_error status = legacy_hal_->getWlanDriverVersion(buffer, sizeof(buffer));
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << LegacyErrorToString(status);
    return Void();
  }
  result.driverDescription = buffer;

  bzero(buffer, sizeof(buffer));
  status = legacy_hal_->getWlanFirmwareVersion(buffer, sizeof(buffer));
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << LegacyErrorToString(status);
    return Void();
  }
  result.firmwareDescription = buffer;

  for (const auto& callback : callbacks_) {
    callback->onChipDebugInfoAvailable(result);
  }
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump() {
  std::vector<uint8_t> driver_dump;
  const auto on_dump_callback = [&driver_dump](char* buffer, int buffer_size) {
    std::vector<uint8_t> dump;
    dump.assign((uint8_t*)buffer, (uint8_t*)buffer + buffer_size);
    driver_dump.insert(driver_dump.end(), dump.begin(), dump.end());
  };
  wifi_error status =
      legacy_hal_->requestWlanDriverMemoryDump(on_dump_callback);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << LegacyErrorToString(status);
    return Void();
  }

  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(driver_dump.data(), driver_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onDriverDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump() {
  std::vector<uint8_t> firmware_dump;
  const auto on_dump_callback = [&firmware_dump](char* buffer,
                                                 int buffer_size) {
    std::vector<uint8_t> dump;
    dump.assign((uint8_t*)buffer, (uint8_t*)buffer + buffer_size);
    firmware_dump.insert(firmware_dump.end(), dump.begin(), dump.end());
  };
  wifi_error status =
      legacy_hal_->requestWlanFirmwareMemoryDump(on_dump_callback);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << LegacyErrorToString(status);
    return Void();
  }

  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(firmware_dump.data(), firmware_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onDriverDebugDumpAvailable(hidl_data);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
