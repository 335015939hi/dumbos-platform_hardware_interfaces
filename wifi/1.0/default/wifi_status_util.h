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

#ifndef WIFI_STATUS_UTIL_H_
#define WIFI_STATUS_UTIL_H_

#include <android/hardware/wifi/1.0/IWifi.h>

#include "wifi_legacy_hal.h"

#define GET_MACRO(_1, _2, NAME, ...) NAME

#define CREATE_WIFI_STATUS2(code, description) \
  createWifiStatus(code, description, __PRETTY_FUNCTION__)
#define CREATE_WIFI_STATUS1(code) \
  createWifiStatus(code, "", __PRETTY_FUNCTION__)
#define CREATE_WIFI_STATUS(...) \
  GET_MACRO(__VA_ARGS__, CREATE_WIFI_STATUS2, CREATE_WIFI_STATUS1)(__VA_ARGS__)

#define CREATE_WIFI_STATUS_FROM_LEGACY_ERROR2(legacy_code, description) \
  createWifiStatusFromLegacyError(legacy_code, description, __PRETTY_FUNCTION__)
#define CREATE_WIFI_STATUS_FROM_LEGACY_ERROR1(legacy_code) \
  createWifiStatusFromLegacyError(legacy_code, "", __PRETTY_FUNCTION__)
#define CREATE_WIFI_STATUS_FROM_LEGACY_ERROR(...)  \
  GET_MACRO(__VA_ARGS__,                           \
            CREATE_WIFI_STATUS_FROM_LEGACY_ERROR2, \
            CREATE_WIFI_STATUS_FROM_LEGACY_ERROR1) \
  (__VA_ARGS__)

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

std::string legacyErrorToString(legacy_hal::wifi_error error);
WifiStatus createWifiStatus(WifiStatusCode code,
                            const std::string& description,
                            const std::string& function_name);
WifiStatus createWifiStatusFromLegacyError(legacy_hal::wifi_error error,
                                           const std::string& description,
                                           const std::string& function_name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_STATUS_UTIL_H_
