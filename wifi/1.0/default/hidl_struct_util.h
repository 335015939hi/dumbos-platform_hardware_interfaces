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
 * This file contains a bunch of functions to convert structs from internal to
 * HIDL and vice versa.
 * TODO(b/32093047): Add unit tests for these conversion methods in the VTS test
 * suite.
 */
namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace internal {

bool convertHidlScanParamsToInternal(
    const IWifiStaIface::BackgroundScanParameters& params,
    wifi_scan_cmd_params* internal_scan_params);

// Convert the blob of packed IE elements to hidl_vec of
// |WifiInformationElement|
// structures.
bool convertInternalIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    hidl_vec<WifiInformationElement>* hidl_ie_elements);

// The scan result contains a variable sized IE info at the
// end for full scan results. So, use the |has_ie_data| flag to
// indicate if the IE info needs to be parsed or not.
bool convertInternalScanResultToHidl(
    const wifi_scan_result* result,
    IWifiStaIfaceEventCallback::ScanResult* hidl_scan_result,
    bool has_ie_data);

bool convertInternalVectorOfCachedScanResultsToHidl(
    const std::vector<wifi_cached_scan_results>& cached_results,
    hidl_vec<IWifiStaIfaceEventCallback::ScanData>* hidl_scan_datas);

bool convertInternalLinkLayerStatsToHidl(
    const android::hardware::wifi::V1_0::implementation::LinkLayerStatsData&
        stats,
    IWifiStaIface::LinkLayerStats* hidl_stats);
}  // namespace internal
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_H_
