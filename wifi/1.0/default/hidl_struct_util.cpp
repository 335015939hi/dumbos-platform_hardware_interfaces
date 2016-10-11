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

#include "hidl_struct_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace internal {

bool convertHidlScanParamsToInternal(
    const IWifiStaIface::BackgroundScanParameters& params,
    wifi_scan_cmd_params* internal_scan_params) {
  if (internal_scan_params == nullptr) {
    return false;
  }
  internal_scan_params->base_period = params.basePeriodInMs;
  internal_scan_params->max_ap_per_scan = params.maxApPerScan;
  internal_scan_params->report_threshold_percent =
      params.reportThresholdPercent;
  internal_scan_params->report_threshold_num_scans =
      params.reportThresholdNumScans;
  // TODO: Expose these max limits in the HIDL interface.
  if (params.buckets.size() > MAX_BUCKETS) {
    return false;
  }
  internal_scan_params->num_buckets = params.buckets.size();
  for (uint32_t bucket_idx = 0; bucket_idx < params.buckets.size();
       bucket_idx++) {
    const IWifiStaIface::BackgroundScanBucketParameters& bucket_spec =
        params.buckets[bucket_idx];
    wifi_scan_bucket_spec& internal_bucket_spec =
        internal_scan_params->buckets[bucket_idx];
    internal_bucket_spec.bucket = bucket_idx;
    internal_bucket_spec.band = static_cast<wifi_band>(bucket_spec.band);
    internal_bucket_spec.period = bucket_spec.periodInMs;
    internal_bucket_spec.max_period = bucket_spec.exponentialMaxPeriodInMs;
    internal_bucket_spec.base = bucket_spec.exponentialBase;
    internal_bucket_spec.step_count = bucket_spec.exponentialStepCount;
    internal_bucket_spec.report_events = 0;
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            IWifiStaIface::BackgroundScanBucketEventReportSchemeMask::
                EACH_SCAN)) {
      internal_bucket_spec.report_events &= REPORT_EVENTS_EACH_SCAN;
    }
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            IWifiStaIface::BackgroundScanBucketEventReportSchemeMask::
                FULL_RESULTS)) {
      internal_bucket_spec.report_events &= REPORT_EVENTS_FULL_RESULTS;
    }
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            IWifiStaIface::BackgroundScanBucketEventReportSchemeMask::
                NO_BATCH)) {
      internal_bucket_spec.report_events &= REPORT_EVENTS_NO_BATCH;
    }
    // TODO: Expose these max limits in the HIDL interface.
    if (bucket_spec.frequenciesInMhz.size() > MAX_CHANNELS) {
      return false;
    }
    internal_bucket_spec.num_channels = bucket_spec.frequenciesInMhz.size();
    for (uint32_t freq_idx = 0; freq_idx < bucket_spec.frequenciesInMhz.size();
         freq_idx++) {
      wifi_scan_channel_spec& internal_channel_spec =
          internal_bucket_spec.channels[freq_idx];
      internal_channel_spec.channel = bucket_spec.frequenciesInMhz[freq_idx];
    }
  }
  return true;
}

bool convertInternalIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    hidl_vec<IWifiStaIfaceEventCallback::InformationElement>*
        hidl_ie_elements) {
  if (ie_blob == nullptr || hidl_ie_elements == nullptr) {
    return false;
  }
  // First convert to a std::vector of IE elements and then push it to a
  // hidl_vec.
  std::vector<IWifiStaIfaceEventCallback::InformationElement>
      hidl_ie_elements_vec;
  const uint8_t* ie_elems_address = ie_blob;
  uint32_t ie_elems_total_len = ie_blob_len;
  uint32_t processed_so_far = 0;

  // Each IE should atleast have the |id| & |len| field.
  while (processed_so_far + sizeof(wifi_information_element) <
         ie_elems_total_len) {
    IWifiStaIfaceEventCallback::InformationElement hidl_ie_element;
    const wifi_information_element* ie_element =
        reinterpret_cast<const wifi_information_element*>(
            &ie_elems_address[processed_so_far]);

    uint32_t curr_ie_elem_len =
        sizeof(wifi_information_element) + ie_element->len;
    if (processed_so_far + curr_ie_elem_len > ie_elems_total_len) {
      return false;
    }
    hidl_ie_element.id = ie_element->id;
    hidl_ie_element.data.setToExternal(
        const_cast<uint8_t*>(
            reinterpret_cast<const uint8_t*>(ie_element->data)),
        ie_element->len);
    hidl_ie_elements_vec.emplace_back(hidl_ie_element);
    processed_so_far += curr_ie_elem_len;
  }
  hidl_ie_elements->setToExternal(hidl_ie_elements_vec.data(),
                                  hidl_ie_elements_vec.size());

  // Ensure that the blob has been fully consumed.
  return (processed_so_far == ie_elems_total_len);
}

// The scan result contains a variable sized IE info at the
// end for full scan results. So, use the |has_ie_data| flag to
// indicate if the IE info needs to be parsed or not.
bool convertInternalScanResultToHidl(
    const wifi_scan_result* result,
    IWifiStaIfaceEventCallback::ScanResult* hidl_scan_result,
    bool has_ie_data) {
  if (result == nullptr || hidl_scan_result == nullptr) {
    return false;
  }
  hidl_scan_result->timeStampInUs = result->ts;
  hidl_scan_result->ssid.setToExternal(
      const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(result->ssid)),
      sizeof(result->ssid));
  for (uint32_t bssid_idx = 0; bssid_idx < hidl_scan_result->bssid.size();
       bssid_idx++) {
    hidl_scan_result->bssid[bssid_idx] = result->bssid[bssid_idx];
  }
  hidl_scan_result->frequencyInMhz = result->channel;
  hidl_scan_result->rssi = result->rssi;
  hidl_scan_result->beaconPeriodInMs = result->beacon_period;
  hidl_scan_result->capability = result->capability;
  // If there is no IE info, the |informationElements| vector is left empty.
  if (has_ie_data) {
    return convertInternalIeBlobToHidl(
        reinterpret_cast<const uint8_t*>(result->ie_data),
        result->ie_length,
        &hidl_scan_result->informationElements);
  }
  return true;
}

bool convertInternalCachedScanResultsToHidl(
    const wifi_cached_scan_results& cached_result,
    IWifiStaIfaceEventCallback::ScanData* hidl_scan_data) {
  if (hidl_scan_data == nullptr) {
    return false;
  }
  hidl_scan_data->flags = cached_result.flags;
  hidl_scan_data->bucketsScanned = cached_result.buckets_scanned;
  // First convert to a std::vector of scan results and then push it to a
  // hidl_vec.
  std::vector<IWifiStaIfaceEventCallback::ScanResult> hidl_scan_results_vec;
  for (uint32_t result_idx = 0;
       result_idx < static_cast<uint32_t>(cached_result.num_results);
       result_idx++) {
    IWifiStaIfaceEventCallback::ScanResult hidl_scan_result;
    if (!convertInternalScanResultToHidl(
            &cached_result.results[result_idx], &hidl_scan_result, false)) {
      return false;
    }
    hidl_scan_results_vec.emplace_back(hidl_scan_result);
  }
  hidl_scan_data->results.setToExternal(hidl_scan_results_vec.data(),
                                        hidl_scan_results_vec.size());
  return true;
}

bool convertInternalVectorOfCachedScanResultsToHidl(
    const std::vector<wifi_cached_scan_results>& cached_results,
    hidl_vec<IWifiStaIfaceEventCallback::ScanData>* hidl_scan_datas) {
  if (hidl_scan_datas == nullptr) {
    return false;
  }
  // First convert to a std::vector of scan datas and then push it to a
  // hidl_vec.
  std::vector<IWifiStaIfaceEventCallback::ScanData> hidl_scan_datas_vec;
  for (const auto& cached_result : cached_results) {
    IWifiStaIfaceEventCallback::ScanData hidl_scan_data;
    if (!convertInternalCachedScanResultsToHidl(cached_result,
                                                &hidl_scan_data)) {
      return false;
    }
    hidl_scan_datas_vec.emplace_back(hidl_scan_data);
  }
  hidl_scan_datas->setToExternal(hidl_scan_datas_vec.data(),
                                 hidl_scan_datas_vec.size());
  return true;
}

}  // namespace internal
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
