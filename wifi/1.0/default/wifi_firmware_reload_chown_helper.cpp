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
#include "wifi_hal/driver_tool.h"

/**
 * Simple suid helper to change ownership of the firmware mode switch sysfs
 * paths to wifi user.
 * Uses DriverTool::TakeOwnershipOfFirmwareReload() to change the
 * ownership. This helps us to let the wifi hal daemon run as wifi user(instead
 * of root).
 */
int main(int /* argc */, char** /* argv */) {
  if (!android::wifi_hal::DriverTool::TakeOwnershipOfFirmwareReload()) {
    LOG(ERROR) << "Failed to take ownership of firmware reload";
    return -1;
  }
  return 0;
}
