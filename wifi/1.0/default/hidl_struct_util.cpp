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

#include <utils/SystemClock.h>

#include "hidl_struct_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_struct_util {

bool convertHidlScanParamsToLegacy(
    const StaBackgroundScanParameters& params,
    legacy_hal::wifi_scan_cmd_params* internal_scan_params) {
  if (!internal_scan_params) {
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
    const StaBackgroundScanBucketParameters& bucket_spec =
        params.buckets[bucket_idx];
    legacy_hal::wifi_scan_bucket_spec& internal_bucket_spec =
        internal_scan_params->buckets[bucket_idx];
    internal_bucket_spec.bucket = bucket_idx;
    internal_bucket_spec.band =
        static_cast<legacy_hal::wifi_band>(bucket_spec.band);
    internal_bucket_spec.period = bucket_spec.periodInMs;
    internal_bucket_spec.max_period = bucket_spec.exponentialMaxPeriodInMs;
    internal_bucket_spec.base = bucket_spec.exponentialBase;
    internal_bucket_spec.step_count = bucket_spec.exponentialStepCount;
    internal_bucket_spec.report_events = 0;
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            StaBackgroundScanBucketEventReportSchemeMask::EACH_SCAN)) {
      internal_bucket_spec.report_events |= REPORT_EVENTS_EACH_SCAN;
    }
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            StaBackgroundScanBucketEventReportSchemeMask::FULL_RESULTS)) {
      internal_bucket_spec.report_events |= REPORT_EVENTS_FULL_RESULTS;
    }
    if (bucket_spec.eventReportScheme &
        static_cast<uint32_t>(
            StaBackgroundScanBucketEventReportSchemeMask::NO_BATCH)) {
      internal_bucket_spec.report_events |= REPORT_EVENTS_NO_BATCH;
    }
    // TODO: Expose these max limits in the HIDL interface.
    if (bucket_spec.frequencies.size() > MAX_CHANNELS) {
      return false;
    }
    internal_bucket_spec.num_channels = bucket_spec.frequencies.size();
    for (uint32_t freq_idx = 0; freq_idx < bucket_spec.frequencies.size();
         freq_idx++) {
      legacy_hal::wifi_scan_channel_spec& internal_channel_spec =
          internal_bucket_spec.channels[freq_idx];
      internal_channel_spec.channel = bucket_spec.frequencies[freq_idx];
    }
  }
  return true;
}

bool convertLegacyIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    std::vector<WifiInformationElement>* hidl_ie_elements) {
  if (!ie_blob || !hidl_ie_elements) {
    return false;
  }
  const uint8_t* ie_elems_address = ie_blob;
  uint32_t ie_elems_total_len = ie_blob_len;
  uint32_t processed_so_far = 0;
  // Each IE should atleast have the |id| & |len| field.
  while (processed_so_far + sizeof(legacy_hal::wifi_information_element) <
         ie_elems_total_len) {
    WifiInformationElement hidl_ie_element;
    const legacy_hal::wifi_information_element* ie_element =
        reinterpret_cast<const legacy_hal::wifi_information_element*>(
            &ie_elems_address[processed_so_far]);

    uint32_t curr_ie_elem_len =
        sizeof(legacy_hal::wifi_information_element) + ie_element->len;
    if (processed_so_far + curr_ie_elem_len > ie_elems_total_len) {
      return false;
    }
    hidl_ie_element.id = ie_element->id;
    hidl_ie_element.data.setToExternal(
        const_cast<uint8_t*>(
            reinterpret_cast<const uint8_t*>(ie_element->data)),
        ie_element->len);
    hidl_ie_elements->emplace_back(hidl_ie_element);
    processed_so_far += curr_ie_elem_len;
  }
  // Ensure that the blob has been fully consumed.
  return (processed_so_far == ie_elems_total_len);
}

bool convertLegacyScanResultToHidl(const legacy_hal::wifi_scan_result* result,
                                   StaScanResult* hidl_scan_result,
                                   bool has_ie_data) {
  if (!result || !hidl_scan_result) {
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
  hidl_scan_result->frequency = result->channel;
  hidl_scan_result->rssi = result->rssi;
  hidl_scan_result->beaconPeriodInMs = result->beacon_period;
  hidl_scan_result->capability = result->capability;
  // If there is no IE info, the |informationElements| vector is left empty.
  if (has_ie_data) {
    std::vector<WifiInformationElement> information_elements;
    if (!convertLegacyIeBlobToHidl(
            reinterpret_cast<const uint8_t*>(result->ie_data),
            result->ie_length,
            &information_elements)) {
      return false;
    }
    hidl_scan_result->informationElements = std::move(information_elements);
  }
  return true;
}

bool convertLegacyCachedScanResultsToHidl(
    const legacy_hal::wifi_cached_scan_results& cached_result,
    StaScanData* hidl_scan_data) {
  if (!hidl_scan_data) {
    return false;
  }
  hidl_scan_data->flags = cached_result.flags;
  hidl_scan_data->bucketsScanned = cached_result.buckets_scanned;

  std::vector<StaScanResult> hidl_scan_results;
  for (uint32_t result_idx = 0;
       result_idx < static_cast<uint32_t>(cached_result.num_results);
       result_idx++) {
    StaScanResult hidl_scan_result;
    if (!convertLegacyScanResultToHidl(
            &cached_result.results[result_idx], &hidl_scan_result, false)) {
      return false;
    }
    hidl_scan_results.emplace_back(hidl_scan_result);
  }
  hidl_scan_data->results = std::move(hidl_scan_results);
  return true;
}

bool convertLegacyVectorOfCachedScanResultsToHidl(
    const std::vector<legacy_hal::wifi_cached_scan_results>& cached_results,
    std::vector<StaScanData>* hidl_scan_datas) {
  if (!hidl_scan_datas) {
    return false;
  }
  for (const auto& cached_result : cached_results) {
    StaScanData hidl_scan_data;
    if (!convertLegacyCachedScanResultsToHidl(cached_result, &hidl_scan_data)) {
      return false;
    }
    hidl_scan_datas->emplace_back(hidl_scan_data);
  }
  return true;
}

bool convertLegacyLinkLayerStatsToHidl(const legacy_hal::LinkLayerStats& stats,
                                       StaLinkLayerStats* hidl_stats) {
  if (!hidl_stats) {
    return false;
  }
  // iface stats conversion.
  hidl_stats->iface.beaconRx = stats.iface.beacon_rx;
  hidl_stats->iface.avgRssiMgmt = stats.iface.rssi_mgmt;
  hidl_stats->iface.wmeBePktStats.rxMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu;
  hidl_stats->iface.wmeBePktStats.txMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu;
  hidl_stats->iface.wmeBePktStats.lostMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost;
  hidl_stats->iface.wmeBePktStats.retries =
      stats.iface.ac[legacy_hal::WIFI_AC_BE].retries;
  hidl_stats->iface.wmeBkPktStats.rxMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu;
  hidl_stats->iface.wmeBkPktStats.txMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu;
  hidl_stats->iface.wmeBkPktStats.lostMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost;
  hidl_stats->iface.wmeBkPktStats.retries =
      stats.iface.ac[legacy_hal::WIFI_AC_BK].retries;
  hidl_stats->iface.wmeViPktStats.rxMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu;
  hidl_stats->iface.wmeViPktStats.txMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu;
  hidl_stats->iface.wmeViPktStats.lostMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost;
  hidl_stats->iface.wmeViPktStats.retries =
      stats.iface.ac[legacy_hal::WIFI_AC_VI].retries;
  hidl_stats->iface.wmeVoPktStats.rxMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu;
  hidl_stats->iface.wmeVoPktStats.txMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu;
  hidl_stats->iface.wmeVoPktStats.lostMpdu =
      stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost;
  hidl_stats->iface.wmeVoPktStats.retries =
      stats.iface.ac[legacy_hal::WIFI_AC_VO].retries;
  // radio stats conversion.
  hidl_stats->radio.onTimeInMs = stats.radio.on_time;
  hidl_stats->radio.txTimeInMs = stats.radio.tx_time;
  hidl_stats->radio.rxTimeInMs = stats.radio.rx_time;
  hidl_stats->radio.onTimeInMsForScan = stats.radio.on_time_scan;
  hidl_stats->radio.txTimeInMsPerLevel.setToExternal(
      const_cast<uint32_t*>(stats.radio_tx_time_per_levels.data()),
      stats.radio_tx_time_per_levels.size());
  // Timestamp in the HAL wrapper here since it's not provided in the legacy
  // HAL API.
  hidl_stats->timeStampInMs = uptimeMillis();
  return true;
}
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
