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

#include "hidl_return_util.h"
#include "hidl_struct_util.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiStaIface::WifiStaIface(
    const std::string& ifname,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiStaIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

Return<void> WifiStaIface::getName(getName_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID, hidl_string());
  }
  hidl_string hidl_ifname;
  hidl_ifname.setToExternal(ifname_.c_str(), ifname_.size());
  HIDL_RETURN1(WifiStatusCode::SUCCESS, hidl_ifname);
}

Return<void> WifiStaIface::getType(getType_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID, IfaceType::STA);
  }
  HIDL_RETURN1(WifiStatusCode::SUCCESS, IfaceType::STA);
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  HIDL_RETURN0(WifiStatusCode::SUCCESS);
}

Return<void> WifiStaIface::getCapabilities(getCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID, 0);
  }

  legacy_hal::wifi_error legacy_status;
  uint32_t feature_set;
  std::tie(legacy_status, feature_set) =
      legacy_hal_.lock()->getSupportedFeatureSet();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status, 0);
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
  legacy_hal::PacketFilterCapabilities apf_caps;
  std::tie(legacy_status, apf_caps) =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status, 0);
  }
  if (apf_caps.version > 0) {
    caps |= static_cast<caps_type>(StaIfaceCapabilityMask::APF);
  }

  HIDL_RETURN1(WifiStatusCode::SUCCESS, caps);
}

Return<void> WifiStaIface::getApfPacketFilterCapabilities(
    getApfPacketFilterCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 ApfPacketFilterCapabilities());
  }

  legacy_hal::wifi_error legacy_status;
  legacy_hal::PacketFilterCapabilities apf_caps;
  std::tie(legacy_status, apf_caps) =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status,
                                   ApfPacketFilterCapabilities());
  }
  ApfPacketFilterCapabilities caps;
  caps.version = apf_caps.version;
  caps.maxLength = apf_caps.max_len;

  HIDL_RETURN1(WifiStatusCode::SUCCESS, caps);
}

Return<void> WifiStaIface::installApfPacketFilter(
    const hidl_vec<uint8_t>& program,
    installApfPacketFilter_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  std::vector<uint8_t> program_vec(&program[0], &program[0] + program.size());
  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->setPacketFilter(program_vec);
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN0_FROM_LEGACY_ERROR(legacy_status);
  } else {
    HIDL_RETURN0(WifiStatusCode::SUCCESS);
  }
}

Return<void> WifiStaIface::getBackgroundScanCapabilities(
    getBackgroundScanCapabilities_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 BackgroundScanCapabilities());
  }

  legacy_hal::wifi_error legacy_status;
  legacy_hal::wifi_gscan_capabilities gscan_caps;
  std::tie(legacy_status, gscan_caps) =
      legacy_hal_.lock()->getGscanCapabilities();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status, BackgroundScanCapabilities());
  }
  BackgroundScanCapabilities caps;
  caps.maxCacheSize = gscan_caps.max_scan_cache_size;
  caps.maxBuckets = gscan_caps.max_scan_buckets;
  caps.maxApCachePerScan = gscan_caps.max_ap_cache_per_scan;
  caps.maxReportingThreshold = gscan_caps.max_scan_reporting_threshold;

  HIDL_RETURN1(WifiStatusCode::SUCCESS, caps);
}

Return<void> WifiStaIface::startBackgroundScan(
    uint32_t cmdId,
    const IWifiStaIface::BackgroundScanParameters& params,
    startBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  legacy_hal::wifi_scan_cmd_params internal_scan_params;
  if (!internal::convertHidlScanParamsToInternal(params,
                                                 &internal_scan_params)) {
    HIDL_RETURN0(WifiStatusCode::ERROR_INVALID_ARGS);
  }

  const auto& on_failure_callback = [&](legacy_hal::wifi_request_id id) {
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundScanFailure(id);
    }
  };
  const auto& on_results_callback = [&](
      legacy_hal::wifi_request_id id,
      const std::vector<legacy_hal::wifi_cached_scan_results>& results) {
    hidl_vec<IWifiStaIfaceEventCallback::ScanData> hidl_scan_datas;
    if (!internal::convertInternalVectorOfCachedScanResultsToHidl(
            results, &hidl_scan_datas)) {
      LOG(ERROR) << "Failed to convert scan results to HIDL structs";
      return;
    }
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundScanResults(id, hidl_scan_datas);
    }
  };
  const auto& on_full_result_callback = [&](
      legacy_hal::wifi_request_id id,
      const legacy_hal::wifi_scan_result* result,
      uint32_t /* buckets_scanned */) {
    IWifiStaIfaceEventCallback::ScanResult hidl_scan_result;
    if (!internal::convertInternalScanResultToHidl(
            result, &hidl_scan_result, true)) {
      LOG(ERROR) << "Failed to convert full scan results to HIDL structs";
      return;
    }
    for (const auto& callback : event_callbacks_) {
      callback->onBackgroundFullScanResult(id, hidl_scan_result);
    }
  };

  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->startGscan(cmdId,
                                     internal_scan_params,
                                     on_failure_callback,
                                     on_results_callback,
                                     on_full_result_callback);
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN0_FROM_LEGACY_ERROR(legacy_status);
  } else {
    HIDL_RETURN0(WifiStatusCode::SUCCESS);
  }
}

Return<void> WifiStaIface::stopBackgroundScan(
    uint32_t cmdId, stopBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->stopGscan(cmdId);
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN0_FROM_LEGACY_ERROR(legacy_status);
  } else {
    HIDL_RETURN0(WifiStatusCode::SUCCESS);
  }
}

Return<void> WifiStaIface::getValidFrequenciesForBackgroundScan(
    IWifiStaIface::BackgroundScanBand band,
    getValidFrequenciesForBackgroundScan_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 hidl_vec<uint32_t>());
  }

  legacy_hal::wifi_error legacy_status;
  std::vector<uint32_t> freqs;
  std::tie(legacy_status, freqs) =
      legacy_hal_.lock()->getValidFrequenciesForGscan(
          static_cast<legacy_hal::wifi_band>(band));
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status, hidl_vec<uint32_t>());
  }
  hidl_vec<uint32_t> hidl_freqs;
  hidl_freqs.setToExternal(freqs.data(), freqs.size());

  HIDL_RETURN1(WifiStatusCode::SUCCESS, hidl_freqs);
}

Return<void> WifiStaIface::enableLinkLayerStatsCollection(
    bool debug, enableLinkLayerStatsCollection_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }

  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->enableLinkLayerStats(debug);
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN0_FROM_LEGACY_ERROR(legacy_status);
  } else {
    HIDL_RETURN0(WifiStatusCode::SUCCESS);
  }
}

Return<void> WifiStaIface::disableLinkLayerStatsCollection(
    disableLinkLayerStatsCollection_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }

  legacy_hal::wifi_error legacy_status =
      legacy_hal_.lock()->disableLinkLayerStats();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN0_FROM_LEGACY_ERROR(legacy_status);
  } else {
    HIDL_RETURN0(WifiStatusCode::SUCCESS);
  }
}

Return<void> WifiStaIface::getLinkLayerStats(
    getLinkLayerStats_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 IWifiStaIface::LinkLayerStats());
  }

  legacy_hal::wifi_error legacy_status;
  legacy_hal::LinkLayerStats stats;
  std::tie(legacy_status, stats) = legacy_hal_.lock()->getLinkLayerStats();
  if (legacy_status != legacy_hal::WIFI_SUCCESS) {
    HIDL_RETURN1_FROM_LEGACY_ERROR(legacy_status,
                                   IWifiStaIface::LinkLayerStats());
  }
  IWifiStaIface::LinkLayerStats hidl_stats;
  if (!internal::convertInternalLinkLayerStatsToHidl(stats, &hidl_stats)) {
    LOG(ERROR) << "Failed to convert link layer stats to HIDL structs";
    HIDL_RETURN1(WifiStatusCode::ERROR_UNKNOWN,
                 IWifiStaIface::LinkLayerStats());
  }

  HIDL_RETURN1(WifiStatusCode::SUCCESS, hidl_stats);
}

Return<void> WifiStaIface::startDebugPacketFateMonitoring(
    startDebugPacketFateMonitoring_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  // TODO: add implementation
  HIDL_RETURN0(WifiStatusCode::SUCCESS);
}

Return<void> WifiStaIface::stopDebugPacketFateMonitoring(
    stopDebugPacketFateMonitoring_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN0(WifiStatusCode::ERROR_WIFI_IFACE_INVALID);
  }
  // TODO: add implementation
  HIDL_RETURN0(WifiStatusCode::SUCCESS);
}

Return<void> WifiStaIface::getDebugTxPacketFates(
    getDebugTxPacketFates_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 hidl_vec<WifiDebugTxPacketFateReport>());
  }
  // TODO: add implementation
  HIDL_RETURN1(WifiStatusCode::SUCCESS,
               hidl_vec<WifiDebugTxPacketFateReport>());
}

Return<void> WifiStaIface::getDebugRxPacketFates(
    getDebugRxPacketFates_cb hidl_status_cb) {
  if (!is_valid_) {
    HIDL_RETURN1(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                 hidl_vec<WifiDebugRxPacketFateReport>());
  }
  // TODO: add implementation
  HIDL_RETURN1(WifiStatusCode::SUCCESS,
               hidl_vec<WifiDebugRxPacketFateReport>());
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
