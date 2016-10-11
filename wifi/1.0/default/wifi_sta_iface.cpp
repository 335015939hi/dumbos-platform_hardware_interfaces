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
  std::pair<wifi_error, PacketFilterCapabilities> ret2 =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (ret2.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret2.first), 0);
    return Void();
  }
  PacketFilterCapabilities& apf_caps = ret2.second;
  if (apf_caps.version > 0) {
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
  std::pair<wifi_error, PacketFilterCapabilities> ret =
      legacy_hal_.lock()->getPacketFilterCapabilities();
  if (ret.first != WIFI_SUCCESS) {
    cb(createWifiStatusFromLegacyError(ret.first),
       ApfPacketFilterCapabilities());
    return Void();
  }
  PacketFilterCapabilities& apf_caps = ret.second;
  ApfPacketFilterCapabilities caps;
  caps.version = apf_caps.version;
  caps.maxLength = apf_caps.max_len;
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
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
