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

#include "wifi_sta_iface.h"

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "wifi_status_util.h"

namespace {
// TODO(b/32093047): Add unit tests for these conversion methods in the VTS test
// suite.
using android::hardware::hidl_vec;
using android::hardware::wifi::V1_0::IWifiStaIface;
using android::hardware::wifi::V1_0::IWifiStaIfaceEventCallback;

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
    if (bucket_spec.frequencies.size() > MAX_CHANNELS) {
      return false;
    }
    internal_bucket_spec.num_channels = bucket_spec.frequencies.size();
    for (uint32_t freq_idx = 0; freq_idx < bucket_spec.frequencies.size();
         freq_idx++) {
      wifi_scan_channel_spec& internal_channel_spec =
          internal_bucket_spec.channels[freq_idx];
      internal_channel_spec.channel = bucket_spec.frequencies[freq_idx];
    }
  }
  return true;
}

// Convert the blob of packed IE elements to hidl_vec of |InformationElement|
// structures.
bool convertInternalIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    hidl_vec<android::hardware::wifi::V1_0::WifiInformationElement>*
        hidl_ie_elements) {
  if (ie_blob == nullptr || hidl_ie_elements == nullptr) {
    return false;
  }
  // First convert to a std::vector of IE elements and then push it to a
  // hidl_vec.
  std::vector<android::hardware::wifi::V1_0::WifiInformationElement>
      hidl_ie_elements_vec;
  const uint8_t* ie_elems_address = ie_blob;
  uint32_t ie_elems_total_len = ie_blob_len;
  uint32_t processed_so_far = 0;

  // Each IE should atleast have the |id| & |len| field.
  while (processed_so_far + sizeof(wifi_information_element) <
         ie_elems_total_len) {
    android::hardware::wifi::V1_0::WifiInformationElement hidl_ie_element;
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
  hidl_scan_result->frequency = result->channel;
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

bool convertInternalLinkLayerStatsToHidl(
    const android::hardware::wifi::V1_0::implementation::LinkLayerStatsData&
        stats,
    IWifiStaIface::LinkLayerStats* hidl_stats) {
  if (hidl_stats == nullptr) {
    return false;
  }
  // iface stats conversion.
  hidl_stats->iface.beaconRx = stats.iface.beacon_rx;
  hidl_stats->iface.avgRssiMgmt = stats.iface.rssi_mgmt;
  hidl_stats->iface.wmeBePktStats.rxMpdu = stats.iface.ac[WIFI_AC_BE].rx_mpdu;
  hidl_stats->iface.wmeBePktStats.txMpdu = stats.iface.ac[WIFI_AC_BE].tx_mpdu;
  hidl_stats->iface.wmeBePktStats.lostMpdu =
      stats.iface.ac[WIFI_AC_BE].mpdu_lost;
  hidl_stats->iface.wmeBePktStats.retries = stats.iface.ac[WIFI_AC_BE].retries;
  hidl_stats->iface.wmeBkPktStats.rxMpdu = stats.iface.ac[WIFI_AC_BK].rx_mpdu;
  hidl_stats->iface.wmeBkPktStats.txMpdu = stats.iface.ac[WIFI_AC_BK].tx_mpdu;
  hidl_stats->iface.wmeBkPktStats.lostMpdu =
      stats.iface.ac[WIFI_AC_BK].mpdu_lost;
  hidl_stats->iface.wmeBkPktStats.retries = stats.iface.ac[WIFI_AC_BK].retries;
  hidl_stats->iface.wmeViPktStats.rxMpdu = stats.iface.ac[WIFI_AC_VI].rx_mpdu;
  hidl_stats->iface.wmeViPktStats.txMpdu = stats.iface.ac[WIFI_AC_VI].tx_mpdu;
  hidl_stats->iface.wmeViPktStats.lostMpdu =
      stats.iface.ac[WIFI_AC_VI].mpdu_lost;
  hidl_stats->iface.wmeViPktStats.retries = stats.iface.ac[WIFI_AC_VI].retries;
  hidl_stats->iface.wmeVoPktStats.rxMpdu = stats.iface.ac[WIFI_AC_VO].rx_mpdu;
  hidl_stats->iface.wmeVoPktStats.txMpdu = stats.iface.ac[WIFI_AC_VO].tx_mpdu;
  hidl_stats->iface.wmeVoPktStats.lostMpdu =
      stats.iface.ac[WIFI_AC_VO].mpdu_lost;
  hidl_stats->iface.wmeVoPktStats.retries = stats.iface.ac[WIFI_AC_VO].retries;
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
  hidl_stats->timeStampInMs = android::uptimeMillis();
  return true;
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiStaIface::WifiStaIface(const std::string& ifname,
                           const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiStaIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiStaIface::getName(getName_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   hidl_string());
    return Void();
  }
  hidl_string hidl_ifname;
  hidl_ifname.setToExternal(ifname_.c_str(), ifname_.size());
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifname);
  return Void();
}

Return<void> WifiStaIface::getType(getType_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   IfaceType::STA);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::STA);
  return Void();
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiStaIface::getCapabilities(getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   0);
    return Void();
  }

  wifi_error legacy_status;
  uint32_t feature_set;
  std::tie(legacy_status, feature_set) =
      legacy_hal_.lock()->getSupportedFeatureSet();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status), 0);
    return Void();
  }

  uint32_t caps = 0;
  typedef std::underlying_type<StaIfaceCapabilityMask>::type caps_type;
  if (feature_set & WIFI_FEATURE_GSCAN) {
    caps |= static_cast<caps_type>(StaIfaceCapabilityMask::BACKGROUND_SCAN);
  }
  if (feature_set & WIFI_FEATURE_LINK_LAYER_STATS) {
    caps |= static_cast<caps_type>(StaIfaceCapabilityMask::LINK_LAYER_STATS);
  }

  // Argh. Currently APF filter capability is implicitly
  // determined by the version provided in APF capabilities.
  // If version is > 0, APF feature is supported.
  PacketFilterCapabilities apf_caps;
  std::tie(legacy_status, apf_caps) =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status), 0);
    return Void();
  }
  if (apf_caps.version > 0) {
    caps |= static_cast<caps_type>(StaIfaceCapabilityMask::APF);
  }

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::getApfPacketFilterCapabilities(
    getApfPacketFilterCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   ApfPacketFilterCapabilities());
    return Void();
  }

  wifi_error legacy_status;
  PacketFilterCapabilities apf_caps;
  std::tie(legacy_status, apf_caps) =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status),
                   ApfPacketFilterCapabilities());
    return Void();
  }
  ApfPacketFilterCapabilities caps;
  caps.version = apf_caps.version;
  caps.maxLength = apf_caps.max_len;

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::installApfPacketFilter(
    const hidl_vec<uint8_t>& program,
    installApfPacketFilter_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  std::vector<uint8_t> program_vec(&program[0], &program[0] + program.size());
  wifi_error status = legacy_hal_.lock()->setPacketFilter(program_vec);
  if (status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(status));
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::getBackgroundScanCapabilities(
    getBackgroundScanCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   BackgroundScanCapabilities());
    return Void();
  }

  wifi_error legacy_status;
  wifi_gscan_capabilities gscan_caps;
  std::tie(legacy_status, gscan_caps) =
      legacy_hal_.lock()->getGscanCapabilities();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status),
                   BackgroundScanCapabilities());
    return Void();
  }
  BackgroundScanCapabilities caps;
  caps.maxCacheSize = gscan_caps.max_scan_cache_size;
  caps.maxBuckets = gscan_caps.max_scan_buckets;
  caps.maxApCachePerScan = gscan_caps.max_ap_cache_per_scan;
  caps.maxReportingThreshold = gscan_caps.max_scan_reporting_threshold;

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::startBackgroundScan(
    uint32_t cmdId,
    const IWifiStaIface::BackgroundScanParameters& params,
    startBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  wifi_scan_cmd_params internal_scan_params;
  if (!convertHidlScanParamsToInternal(params, &internal_scan_params)) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS));
    return Void();
  }

  const auto& on_failure_callback = [&](wifi_request_id id) {
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundScanFailure(id);
    }
  };
  const auto& on_results_callback = [&](
      wifi_request_id id,
      const std::vector<wifi_cached_scan_results>& results) {
    hidl_vec<IWifiStaIfaceEventCallback::ScanData> hidl_scan_datas;
    if (!convertInternalVectorOfCachedScanResultsToHidl(results,
                                                        &hidl_scan_datas)) {
      LOG(ERROR) << "Failed to convert scan results to HIDL structs";
      return;
    }
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundScanResults(id, hidl_scan_datas);
    }
  };
  const auto& on_full_result_callback = [&](wifi_request_id id,
                                            const wifi_scan_result* result,
                                            uint32_t /* buckets_scanned */) {
    IWifiStaIfaceEventCallback::ScanResult hidl_scan_result;
    if (!convertInternalScanResultToHidl(result, &hidl_scan_result, true)) {
      LOG(ERROR) << "Failed to convert full scan results to HIDL structs";
      return;
    }
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundFullScanResult(id, hidl_scan_result);
    }
  };

  wifi_error legacy_status =
      legacy_hal_.lock()->startGscan(cmdId,
                                     internal_scan_params,
                                     on_failure_callback,
                                     on_results_callback,
                                     on_full_result_callback);
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status));
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::stopBackgroundScan(
    uint32_t cmdId, stopBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  wifi_error legacy_status = legacy_hal_.lock()->stopGscan(cmdId);
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status));
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::getValidFrequenciesForBackgroundScan(
    IWifiStaIface::BackgroundScanBand band,
    getValidFrequenciesForBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   hidl_vec<uint32_t>());
    return Void();
  }

  wifi_error legacy_status;
  std::vector<uint32_t> freqs;
  std::tie(legacy_status, freqs) =
      legacy_hal_.lock()->getValidFrequenciesForGscan(
          static_cast<wifi_band>(band));
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status),
                   hidl_vec<uint32_t>());
    return Void();
  }
  hidl_vec<uint32_t> hidl_freqs;
  hidl_freqs.setToExternal(freqs.data(), freqs.size());

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_freqs);
  return Void();
}

Return<void> WifiStaIface::enableLinkLayerStatsCollection(
    bool debug, enableLinkLayerStatsCollection_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }

  wifi_error legacy_status = legacy_hal_.lock()->enableLinkLayerStats(debug);
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status));
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::disableLinkLayerStatsCollection(
    disableLinkLayerStatsCollection_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }

  wifi_error legacy_status = legacy_hal_.lock()->disableLinkLayerStats();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status));
  } else {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::getLinkLayerStats(
    getLinkLayerStats_cb hidl_status_cb) {
  if (!is_valid_) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
                   IWifiStaIface::LinkLayerStats());
    return Void();
  }

  wifi_error legacy_status;
  LinkLayerStatsData stats;
  std::tie(legacy_status, stats) = legacy_hal_.lock()->getLinkLayerStats();
  if (legacy_status != WIFI_SUCCESS) {
    hidl_status_cb(createWifiStatusFromLegacyError(legacy_status),
                   IWifiStaIface::LinkLayerStats());
    return Void();
  }
  IWifiStaIface::LinkLayerStats hidl_stats;
  if (!convertInternalLinkLayerStatsToHidl(stats, &hidl_stats)) {
    LOG(ERROR) << "Failed to convert link layer stats to HIDL structs";
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_UNKNOWN),
                   IWifiStaIface::LinkLayerStats());
    return Void();
  }

  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_stats);
  return Void();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
