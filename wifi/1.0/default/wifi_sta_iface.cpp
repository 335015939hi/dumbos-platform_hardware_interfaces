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

// Convert the blob of packed IE elements to hidl_vec of |InformationElement|
// structures.
bool convertInternalIeBlobToHidl(
    const uint8_t* ie_blob,
    uint32_t ie_blob_len,
    hidl_vec<IWifiStaIfaceEventCallback::InformationElement>*
        hidl_ie_elements) {
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
  hidl_ie_elements->setToExternal(
      hidl_ie_elements_vec.data(),
      hidl_ie_elements_vec.size() *
          sizeof(IWifiStaIfaceEventCallback::InformationElement));

  // Ensure that the blob has been fully consumed.
  return (processed_so_far == ie_elems_total_len);
}

bool convertInternalScanResultToHidl(
    const wifi_scan_result& result,
    IWifiStaIfaceEventCallback::ScanResult* hidl_scan_result) {
  hidl_scan_result->timeStampInUs = result.ts;
  hidl_scan_result->ssid.setToExternal(
      const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(result.ssid)),
      sizeof(result.ssid));
  for (uint32_t bssid_idx = 0; bssid_idx < hidl_scan_result->bssid.size();
       bssid_idx++) {
    hidl_scan_result->bssid[bssid_idx] = result.bssid[bssid_idx];
  }
  hidl_scan_result->frequencyInMhz = result.channel;
  hidl_scan_result->rssi = result.rssi;
  hidl_scan_result->beaconPeriodInMs = result.beacon_period;
  hidl_scan_result->capability = result.capability;
  return convertInternalIeBlobToHidl(
      reinterpret_cast<const uint8_t*>(result.ie_data),
      result.ie_length,
      &hidl_scan_result->informationElements);
}

bool convertInternalCachedScanResultsToHidl(
    const wifi_cached_scan_results& cached_result,
    IWifiStaIfaceEventCallback::ScanData* hidl_scan_data) {
  hidl_scan_data->flags = cached_result.flags;
  hidl_scan_data->bucketsScanned = cached_result.buckets_scanned;
  // First convert to a std::vector of scan results and then push it to a
  // hidl_vec.
  std::vector<IWifiStaIfaceEventCallback::ScanResult> hidl_scan_results_vec;
  for (uint32_t result_idx = 0;
       result_idx < static_cast<uint32_t>(cached_result.num_results);
       result_idx++) {
    IWifiStaIfaceEventCallback::ScanResult hidl_scan_result;
    if (!convertInternalScanResultToHidl(cached_result.results[result_idx],
                                         &hidl_scan_result)) {
      return false;
    }
    hidl_scan_results_vec.emplace_back(hidl_scan_result);
  }
  hidl_scan_data->results.setToExternal(
      hidl_scan_results_vec.data(),
      hidl_scan_results_vec.size() *
          sizeof(IWifiStaIfaceEventCallback::ScanResult));
  return true;
}

bool convertInternalVectorOfCachedScanResultsToHidl(
    const std::vector<wifi_cached_scan_results>& cached_results,
    hidl_vec<IWifiStaIfaceEventCallback::ScanData>* hidl_scan_datas) {
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
  hidl_scan_datas->setToExternal(
      hidl_scan_datas_vec.data(),
      hidl_scan_datas_vec.size() *
          sizeof(IWifiStaIfaceEventCallback::ScanData));
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
  callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiStaIface::getName(getName_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
       hidl_string());
    return Void();
  }
  hidl_string hidl_ifname;
  hidl_ifname.setToExternal(ifname_.c_str(), ifname_.size());
  cb(createWifiStatus(WifiStatusCode::SUCCESS), hidl_ifname);
  return Void();
}

Return<void> WifiStaIface::getType(getType_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
       IfaceType::STA);
    return Void();
  }
  cb(createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::STA);
  return Void();
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& callback,
    registerEventCallback_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.emplace_back(callback);
  cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> WifiStaIface::getCapabilities(getCapabilities_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID), 0);
    return Void();
  }
  std::pair<wifi_error, uint32_t> ret1 =
      legacy_hal_.lock()->getSupportedFeatureSet();
  if (ret1.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret1.first), 0);
    return Void();
  }
  uint32_t& feature_set = ret1.second;
  uint32_t caps = 0;
  if (feature_set & WIFI_FEATURE_GSCAN) {
    caps &= static_cast<uint32_t>(StaIfaceCapabilityMask::BACKGROUND_SCAN);
  }
  // Argh. Currently APF filter capability is implicitly
  // determined by the version provided in APF capabilities.
  // If version is > 0, APF feature is supported.
  std::pair<wifi_error, std::pair<uint32_t, uint32_t>> ret2 =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (ret2.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret2.first), 0);
    return Void();
  }
  uint32_t& version = ret2.second.first;
  if (version > 0) {
    caps &= static_cast<uint32_t>(StaIfaceCapabilityMask::APF);
  }
  cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::getApfPacketFilterCapabilities(
    getApfPacketFilterCapabilities_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
       ApfPacketFilterCapabilities());
    return Void();
  }
  std::pair<wifi_error, std::pair<uint32_t, uint32_t>> ret =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (ret.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret.first),
       ApfPacketFilterCapabilities());
    return Void();
  }
  ApfPacketFilterCapabilities caps;
  caps.version = ret.second.first;
  caps.maxLength = ret.second.second;
  cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::getBackgroundScanCapabilities(
    getBackgroundScanCapabilities_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID),
       BackgroundScanCapabilities());
    return Void();
  }
  std::pair<wifi_error, wifi_gscan_capabilities> ret =
      legacy_hal_.lock()->getGscanCapabilities();
  if (ret.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret.first),
       BackgroundScanCapabilities());
    return Void();
  }
  wifi_gscan_capabilities& gscan_caps = ret.second;
  BackgroundScanCapabilities caps;
  caps.maxCacheSize = gscan_caps.max_scan_cache_size;
  caps.maxBuckets = gscan_caps.max_scan_buckets;
  caps.maxApCachePerScan = gscan_caps.max_ap_cache_per_scan;
  caps.maxReportingThreshold = gscan_caps.max_scan_reporting_threshold;
  cb(createWifiStatus(WifiStatusCode::SUCCESS), caps);
  return Void();
}

Return<void> WifiStaIface::startBackgroundScan(
    uint32_t cmdId,
    const IWifiStaIface::BackgroundScanParameters& params,
    startBackgroundScan_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  wifi_scan_cmd_params internal_scan_params;
  if (!convertHidlScanParamsToInternal(params, &internal_scan_params)) {
    cb(createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS));
    return Void();
  }

  const auto& on_failure_callback = [&](wifi_request_id id) {
    for (const auto& callback : callbacks_) {
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
    for (const auto& callback : callbacks_) {
      callback->onBackgroundScanResults(id, hidl_scan_datas);
    }
  };
  const auto& on_full_result_callback = [&](wifi_request_id id,
                                            const wifi_scan_result& result,
                                            uint32_t /* buckets_scanned */) {
    IWifiStaIfaceEventCallback::ScanResult hidl_scan_result;
    if (!convertInternalScanResultToHidl(result, &hidl_scan_result)) {
      LOG(ERROR) << "Failed to convert full scan results to HIDL structs";
      return;
    }
    for (const auto& callback : callbacks_) {
      callback->onBackgroundFullScanResult(id, hidl_scan_result);
    }
  };

  wifi_error status = legacy_hal_.lock()->startGscan(cmdId,
                                                     internal_scan_params,
                                                     on_failure_callback,
                                                     on_results_callback,
                                                     on_full_result_callback);
  if (status != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(status));
  } else {
    cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}

Return<void> WifiStaIface::stopBackgroundScan(uint32_t cmdId,
                                              stopBackgroundScan_cb cb) {
  if (!is_valid_) {
    cb(createWifiStatus(WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
    return Void();
  }
  wifi_error status = legacy_hal_.lock()->stopGscan(cmdId);
  if (status != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(status));
  } else {
    cb(createWifiStatus(WifiStatusCode::SUCCESS));
  }
  return Void();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
