/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

// Macros to invoke the hidl_status_cb callback to pass status along with any
// values and then return from the method.
#include "wifi_status_util.h"

#define GET_MACRO_OVERLOAD0(arg1, arg2, macro_name, ...) macro_name
#define GET_MACRO_OVERLOAD1(arg1, arg2, arg3, macro_name, ...) macro_name

/**
 * All the combination of macros for HIDL methods with status no return values.
 */
// HIDL method with no return values and status description.
#define HIDL_RETURN0_DESC_WITH_CB(status_code, status_description)         \
  ({                                                                       \
    WifiStatus status = createWifiStatus(status_code, status_description); \
    hidl_status_cb(status);                                                \
    return Void();                                                         \
  })

// HIDL method with no return values and no status description.
#define HIDL_RETURN0_NODESC_WITH_CB(status_code) \
  HIDL_RETURN0_DESC_WITH_CB(status_code, "");

#define HIDL_RETURN0_WITH_CB(...)                                          \
  GET_MACRO_OVERLOAD0(                                                     \
      __VA_ARGS__, HIDL_RETURN0_DESC_WITH_CB, HIDL_RETURN0_NODESC_WITH_CB) \
  (__VA_ARGS__)

// HIDL method with no return values and status description from legacy error
// code.
#define HIDL_RETURN0_DESC_WITH_CB_FROM_LEGACY_ERROR(legacy_status_code,      \
                                                    status_description)      \
  ({                                                                         \
    WifiStatus status = createWifiStatusFromLegacyError(legacy_status_code,  \
                                                        status_description); \
    hidl_status_cb(status);                                                  \
    return Void();                                                           \
  })

// HIDL method with no return values and no status description from legacy error
// code.
#define HIDL_RETURN0_NODESC_WITH_CB_FROM_LEGACY_ERROR(legacy_status_code) \
  HIDL_RETURN0_DESC_WITH_CB_FROM_LEGACY_ERROR(legacy_status_code, "");

#define HIDL_RETURN0_WITH_CB_FROM_LEGACY_ERROR(...)                  \
  GET_MACRO_OVERLOAD0(__VA_ARGS__,                                   \
                      HIDL_RETURN0_DESC_WITH_CB_FROM_LEGACY_ERROR,   \
                      HIDL_RETURN0_NODESC_WITH_CB_FROM_LEGACY_ERROR) \
  (__VA_ARGS__)

/**
 * All the combination of macros for HIDL methods with status and a single
 * return value.
 */
// HIDL method with 1 return value and status description.
#define HIDL_RETURN1_DESC_WITH_CB(status_code, status_description, ret_value) \
  ({                                                                          \
    WifiStatus status = createWifiStatus(status_code, status_description);    \
    hidl_status_cb(status, ret_value);                                        \
    return Void();                                                            \
  })

// HIDL method with 1 return value and no status description.
#define HIDL_RETURN1_NODESC_WITH_CB(status_code, ret_value) \
  HIDL_RETURN1_DESC_WITH_CB(status_code, "", ret_value);

#define HIDL_RETURN1_WITH_CB(...)                                          \
  GET_MACRO_OVERLOAD1(                                                     \
      __VA_ARGS__, HIDL_RETURN1_DESC_WITH_CB, HIDL_RETURN1_NODESC_WITH_CB) \
  (__VA_ARGS__)

// HIDL method with 1 return value and status description from legacy error
// code.
#define HIDL_RETURN1_DESC_WITH_CB_FROM_LEGACY_ERROR(                         \
    legacy_status_code, status_description, ret_value)                       \
  ({                                                                         \
    WifiStatus status = createWifiStatusFromLegacyError(legacy_status_code,  \
                                                        status_description); \
    hidl_status_cb(status, ret_value);                                       \
    return Void();                                                           \
  })

// HIDL method with 1 return value and no status description from legacy error
// code.
#define HIDL_RETURN1_NODESC_WITH_CB_FROM_LEGACY_ERROR(legacy_status_code, \
                                                      ret_value)          \
  HIDL_RETURN1_DESC_WITH_CB_FROM_LEGACY_ERROR(                            \
      legacy_status_code, "", ret_value);

#define HIDL_RETURN1_WITH_CB_FROM_LEGACY_ERROR(...)                  \
  GET_MACRO_OVERLOAD1(__VA_ARGS__,                                   \
                      HIDL_RETURN1_DESC_WITH_CB_FROM_LEGACY_ERROR,   \
                      HIDL_RETURN1_NODESC_WITH_CB_FROM_LEGACY_ERROR) \
  (__VA_ARGS__)
