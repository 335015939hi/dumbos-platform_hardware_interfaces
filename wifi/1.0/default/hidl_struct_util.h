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

#ifndef HIDL_STRUCT_UTIL_H_
#define HIDL_STRUCT_UTIL_H_

#include <android/hardware/wifi/1.0/IWifi.h>

#include "wifi_legacy_hal.h"

/**
 * This file contains a bunch of functions to convert structs from the legacy
 * HAL to HIDL and vice versa.
 * TODO(b/32093047): Add unit tests for these conversion methods in the VTS test
 * suite.
 */
namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_struct_util {

// Convert hidl gscan params to legacy gscan params.
bool convertHidlScanParamsToLegacy(
    const StaBackgroundScanParameters& params,
    legacy_hal::wifi_scan_cmd_params* internal_scan_params);
// Convert the blob of packed IE elements to vector of
// |WifiInformationElement| structures.
bool convertLegacyIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    std::vector<WifiInformationElement>* hidl_ie_elements);
// Convert the legacy full scan results to HIDL |StaScanResult| struct.
// The scan result contains a variable sized IE info at the
// end for full scan results. So, use the |has_ie_data| flag to
// indicate if the IE info needs to be parsed or not.
bool convertLegacyScanResultToHidl(const legacy_hal::wifi_scan_result* result,
                                   StaScanResult* hidl_scan_result,
                                   bool has_ie_data);
// Convert the vector of legacy cached scan results to vector of HIDL
// |StaScanData| struct.
// The cached scan result do not contain IE info. So, this will internally
// invoke |convertLegacyScanResultToHidl| with |has_ie_data| flag set to false
// to parse out the individual scan results in the array.
bool convertLegacyVectorOfCachedScanResultsToHidl(
    const std::vector<legacy_hal::wifi_cached_scan_results>& cached_results,
    std::vector<StaScanData>* hidl_scan_datas);
// Convert legacy LinkLayeerStats struct to HIDL |StaLinkLayerStats| struct.
bool convertLegacyLinkLayerStatsToHidl(const legacy_hal::LinkLayerStats& stats,
                                       StaLinkLayerStats* hidl_stats);
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_H_
