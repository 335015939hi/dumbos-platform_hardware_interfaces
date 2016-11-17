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
#include <android-base/macros.h>
#include <libminijail.h>
#include <private/android_filesystem_config.h>

#include "wifi_mode_controller.h"

using android::hardware::wifi::V1_0::IfaceType;
using android::wifi_hal::DriverTool;

namespace {
bool dropPrivileges() {
  minijail* j = minijail_new();
  if (minijail_change_user(j, "wifi") != 0) {
    return false;
  }
  if (minijail_change_group(j, "wifi") != 0) {
    return false;
  }
  minijail_use_caps(j, CAP_TO_MASK(CAP_NET_ADMIN) | CAP_TO_MASK(CAP_NET_RAW));
  minijail_enter(j);
  minijail_destroy(j);
  return true;
}

int convertIfaceTypeToFirmwareMode(IfaceType type) {
  int mode;
  switch (type) {
    case IfaceType::AP:
      mode = DriverTool::kFirmwareModeAp;
      break;
    case IfaceType::P2P:
      mode = DriverTool::kFirmwareModeP2p;
      break;
    case IfaceType::NAN:
      // NAN is exposed in STA mode currently.
      mode = DriverTool::kFirmwareModeSta;
      break;
    case IfaceType::STA:
      mode = DriverTool::kFirmwareModeSta;
      break;
  }
  return mode;
}
}

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiModeController::WifiModeController() : driver_tool_(new DriverTool) {}

bool WifiModeController::initializeAndDropPriviliges() {
  // Take ownership of firmware reload.
  if (!DriverTool::TakeOwnershipOfFirmwareReload()) {
    return false;
  }
  // Drop the root priviliges of the HAL process.
  return dropPrivileges();
}

bool WifiModeController::isFirmwareModeChangeNeeded(IfaceType type) {
  return driver_tool_->IsFirmwareModeChangeNeeded(
      convertIfaceTypeToFirmwareMode(type));
}

bool WifiModeController::changeFirmwareMode(IfaceType type) {
  return driver_tool_->IsFirmwareModeChangeNeeded(
      convertIfaceTypeToFirmwareMode(type));
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
