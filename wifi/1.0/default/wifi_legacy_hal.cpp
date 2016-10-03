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

#include "wifi_legacy_hal.h"
#include "failure_reason_util.h"

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <wifi_system/hal_tool.h>
#include <wifi_system/interface_tool.h>

namespace {
std::string getWlanInterfaceName() {
  char buffer[PROPERTY_VALUE_MAX];
  property_get("wifi.interface", buffer, "wlan0");
  return buffer;
}

// Legacy HAL functions accept "C" style function pointers, so use global
// functions to pass to the legacy HAL function and store the corresponding
// std::function methods to be invoked.
// Callback to be invoked once |stop| is complete.
std::function<void(wifi_handle handle)> on_stop_complete_callback_ = nullptr;
void onStopComplete(wifi_handle handle) {
  if (on_stop_complete_callback_) {
    on_stop_complete_callback_(handle);
  }
}

// Callback to be invoked for driver dump.
std::function<void(char*, int)> on_driver_memory_dump_callback_ = nullptr;
void onDriverMemoryDump(char* buffer, int buffer_size) {
  if (on_driver_memory_dump_callback_) {
    on_driver_memory_dump_callback_(buffer, buffer_size);
  }
}

// Callback to be invoked for firmware dump.
std::function<void(char*, int)> on_firmware_memory_dump_callback_ = nullptr;
void onFirmwareMemoryDump(char* buffer, int buffer_size) {
  if (on_firmware_memory_dump_callback_) {
    on_firmware_memory_dump_callback_(buffer, buffer_size);
  }
}
}

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiLegacyHal::WifiLegacyHal()
    : global_handle_(nullptr),
      wlan_interface_handle_(nullptr),
      awaiting_event_loop_termination_(false) {}

wifi_error WifiLegacyHal::start() {
  // Ensure that we're starting in a good state.
  CHECK(!global_handle_ && !wlan_interface_handle_ &&
        !awaiting_event_loop_termination_);

  android::wifi_system::HalTool hal_tool;
  android::wifi_system::InterfaceTool if_tool;
  if (!hal_tool.InitFunctionTable(&global_func_table_)) {
    LOG(ERROR) << "Failed to initialize legacy hal function table";
    return WIFI_ERROR_UNKNOWN;
  }
  if (!if_tool.SetWifiUpState(true)) {
    LOG(ERROR) << "Failed to set WiFi interface up";
    return WIFI_ERROR_UNKNOWN;
  }

  LOG(INFO) << "Starting legacy HAL";
  wifi_error status = global_func_table_.wifi_initialize(&global_handle_);
  if (status != WIFI_SUCCESS || !global_handle_) {
    LOG(ERROR) << "Failed to retrieve global handle";
    return status;
  }
  event_loop_thread_ = std::thread(&WifiLegacyHal::runEventLoop, this);
  status = retrieveWlanInterfaceHandle();
  if (status != WIFI_SUCCESS || !wlan_interface_handle_) {
    LOG(ERROR) << "Failed to retrieve wlan interface handle";
    return status;
  }
  LOG(VERBOSE) << "Legacy HAL start complete";
  return WIFI_SUCCESS;
}

wifi_error WifiLegacyHal::stop(
    const std::function<void()>& on_complete_callback) {
  LOG(INFO) << "Stopping legacy HAL";
  on_stop_complete_callback_ = [&](wifi_handle handle) {
    CHECK_EQ(global_handle_, handle) << "Handle mismatch";
    on_complete_callback();
    global_handle_ = nullptr;
    wlan_interface_handle_ = nullptr;
    on_stop_complete_callback_ = nullptr;
  };
  awaiting_event_loop_termination_ = true;
  global_func_table_.wifi_cleanup(global_handle_, onStopComplete);
  LOG(VERBOSE) << "Legacy HAL stop initiated";
  return WIFI_SUCCESS;
}

wifi_error WifiLegacyHal::getWlanDriverVersion(char* buffer, int buffer_size) {
  return global_func_table_.wifi_get_driver_version(
      wlan_interface_handle_, buffer, buffer_size);
}

wifi_error WifiLegacyHal::getWlanFirmwareVersion(char* buffer,
                                                 int buffer_size) {
  return global_func_table_.wifi_get_firmware_version(
      wlan_interface_handle_, buffer, buffer_size);
}

wifi_error WifiLegacyHal::requestWlanDriverMemoryDump(
    std::function<void(char*, int)> on_dump_callback) {
  on_driver_memory_dump_callback_ = on_dump_callback;
  wifi_error status = global_func_table_.wifi_get_driver_memory_dump(
      wlan_interface_handle_, {onDriverMemoryDump});
  on_driver_memory_dump_callback_ = nullptr;
  return status;
}

wifi_error WifiLegacyHal::requestWlanFirmwareMemoryDump(
    std::function<void(char*, int)> on_dump_callback) {
  on_firmware_memory_dump_callback_ = on_dump_callback;
  wifi_error status = global_func_table_.wifi_get_firmware_memory_dump(
      wlan_interface_handle_, {onFirmwareMemoryDump});
  on_firmware_memory_dump_callback_ = nullptr;
  return status;
}

wifi_error WifiLegacyHal::retrieveWlanInterfaceHandle() {
  int num_iface_handles = 0;
  const std::string& wlan_ifname = getWlanInterfaceName();

  wifi_interface_handle* iface_handles = nullptr;
  wifi_error status = global_func_table_.wifi_get_ifaces(
      global_handle_, &num_iface_handles, &iface_handles);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to enumerate interface handles: "
               << LegacyErrorToString(status);
    return status;
  }
  char ifname[IFNAMSIZ];
  for (int i = 0; i < num_iface_handles; ++i) {
    bzero(ifname, sizeof(ifname));
    status = global_func_table_.wifi_get_iface_name(
        iface_handles[i], ifname, sizeof(ifname));
    if (status != WIFI_SUCCESS) {
      LOG(WARNING) << "Failed to get interface handle name: "
                   << LegacyErrorToString(status);
      continue;
    }
    if (wlan_ifname == ifname) {
      wlan_interface_handle_ = iface_handles[i];
      return WIFI_SUCCESS;
    }
  }
  return WIFI_ERROR_UNKNOWN;
}

void WifiLegacyHal::runEventLoop() {
  LOG(VERBOSE) << "Starting legacy HAL event loop";
  global_func_table_.wifi_event_loop(global_handle_);
  if (!awaiting_event_loop_termination_) {
    LOG(FATAL) << "Legacy HAL event loop terminated, but HAL was not stopping";
  }
  LOG(VERBOSE) << "Legacy HAL event loop terminated";
  awaiting_event_loop_termination_ = false;
  android::wifi_system::InterfaceTool if_tool;
  if_tool.SetWifiUpState(false);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
