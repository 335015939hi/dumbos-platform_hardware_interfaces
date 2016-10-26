/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HIDL_RETURN_UTIL_H_
#define HIDL_RETURN_UTIL_H_

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_return_util {
/**
 * These utility functions are used to invoke a method on the provided
 * HIDL interface object.
 * These functions checks if the provided HIDL interface object is valid.
 * a) if valid, Invokes the corresponding internal implementation function of
 * the HIDL method. It then invokes the HIDL continuation callback with
 * the status and any returned values.
 * b) if invalid, invokes the HIDL continuation callback with the
 * provided error status and default values.
 */
// Use for HIDL methods which return only an instance of WifiStatus.
template <typename ObjT, typename WorkFuncT, typename... Args>
Return<void> ValidateAndCall0(ObjT* obj,
                              WifiStatusCode status_code_if_invalid,
                              WorkFuncT&& work,
                              const std::function<void(WifiStatus)>& hidl_cb,
                              Args... args) {
  if (obj->isValid()) {
    hidl_cb((obj->*work)(std::forward<Args>(args)...));
  } else {
    hidl_cb(createWifiStatus(status_code_if_invalid));
  }
  return Void();
}

// Use for HIDL methods which return instance of WifiStatus and a single return
// value.
template <typename ObjT, typename WorkFuncT, typename ReturnT, typename... Args>
Return<void> ValidateAndCall1(
    WifiStatusCode status_code_if_invalid,
    ObjT* obj,
    WorkFuncT&& work,
    const std::function<void(WifiStatus, ReturnT)>& hidl_cb,
    Args... args) {
  if (obj->isValid()) {
    const auto& ret = (obj->*work)(std::forward<Args>(args)...);
    hidl_cb(std::get<0>(ret), std::get<1>(ret));
  } else {
    hidl_cb(createWifiStatus(status_code_if_invalid), ReturnT());
  }
  return Void();
}

// Use for HIDL methods which return instance of WifiStatus and 2 return
// values.
template <typename ObjT,
          typename WorkFuncT,
          typename ReturnT1,
          typename ReturnT2,
          typename... Args>
Return<void> ValidateAndCall2(
    WifiStatusCode status_code_if_invalid,
    ObjT* obj,
    WorkFuncT&& work,
    const std::function<void(WifiStatus, ReturnT1, ReturnT2)>& hidl_cb,
    Args... args) {
  if (obj->isValid()) {
    const auto& ret = (obj->*work)(std::forward<Args>(args)...);
    hidl_cb(std::get<0>(ret), std::get<1>(ret), std::get<2>(ret));
  } else {
    hidl_cb(createWifiStatus(status_code_if_invalid), ReturnT1(), ReturnT2());
  }
  return Void();
}

}  // namespace hidl_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
#endif  // HIDL_RETURN_UTIL_H_
