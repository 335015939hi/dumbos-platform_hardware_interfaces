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

#ifndef WIFI_1_1_H_
#define WIFI_1_1_H_

#include "../../1.0/default/wifi.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_1 {
namespace implementation {

/**
 * Root HIDL interface object used to control the Wifi HAL.
 */
class Wifi : public android::hardware::wifi::V1_0::implementation::Wifi {
 public:
  Wifi();

 protected:
   virtual WifiChip allocateWifiChip(ChipId chipId,
         std::shared_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
         std::shared_ptr<mode_controller::WifiModeController> mode_controller);
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_1_1_H_
