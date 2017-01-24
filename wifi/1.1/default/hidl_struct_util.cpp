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

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "hidl_struct_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_1 {
namespace implementation {
namespace hidl_struct_util {

NanStatusType convertLegacyNanStatusTypeToHidl(
    legacy_hal::NanStatusType type) {
  // values are identical - may need to do a mapping if they diverge in the future
  return (NanStatusType) type;
}

bool convertHidlNanEnableRequestToLegacy(
    const NanEnableRequest& hidl_request,
    legacy_hal::NanEnableRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanEnableRequest));

  // TODO: b/34059183 tracks missing configurations in legacy HAL or uknown defaults
  legacy_request->config_2dot4g_support = 1;
  legacy_request->support_2dot4g_val = hidl_request.operateInBand[
        (size_t) NanBandIndex::NAN_BAND_24GHZ];
  legacy_request->config_support_5g = 1;
  legacy_request->support_5g_val = hidl_request.operateInBand[(size_t) NanBandIndex::NAN_BAND_5GHZ];
  legacy_request->config_hop_count_limit = 0; // TODO: don't know default yet
  legacy_request->hop_count_limit_val = hidl_request.hopCountMax;
  legacy_request->master_pref = hidl_request.configParams.masterPref;
  legacy_request->discovery_indication_cfg = 0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.configParams.disableDiscoveryAddressChangeIndication ? 0x1 : 0x0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.configParams.disableStartedClusterIndication ? 0x2 : 0x0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.configParams.disableJoinedClusterIndication ? 0x4 : 0x0;
  legacy_request->config_sid_beacon = 1;
  if (hidl_request.configParams.numberOfServiceIdsInBeacon > 127) {
    return false;
  }
  legacy_request->sid_beacon_val = (hidl_request.configParams.includeServiceIdsInBeacon ? 0x1 : 0x0)
        | (hidl_request.configParams.numberOfServiceIdsInBeacon << 1);
  legacy_request->config_rssi_window_size = 0; // TODO: don't know default yet
  legacy_request->rssi_window_size_val = hidl_request.configParams.rssiWindowSize;
  legacy_request->config_disc_mac_addr_randomization = 1;
  legacy_request->disc_mac_addr_rand_interval_sec =
        hidl_request.configParams.macAddressRandomizationIntervalSec;
  legacy_request->config_responder_auto_response = 1;
  legacy_request->ranging_auto_response_cfg = hidl_request.configParams.acceptRangingRequests ?
       legacy_hal::NAN_RANGING_AUTO_RESPONSE_ENABLE : legacy_hal::NAN_RANGING_AUTO_RESPONSE_DISABLE;
  legacy_request->config_2dot4g_rssi_close = 0; // TODO: don't know default yet
  legacy_request->rssi_close_2dot4g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiClose;
  legacy_request->config_2dot4g_rssi_middle = 0; // TODO: don't know default yet
  legacy_request->rssi_middle_2dot4g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiMiddle;
  legacy_request->config_2dot4g_rssi_proximity = 0; // TODO: don't know default yet
  legacy_request->rssi_proximity_2dot4g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiProximity;
  legacy_request->config_scan_params = 0; // TODO: don't know default yet
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_24G_BAND] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_24G_BAND] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].scanPeriodSec;
  legacy_request->config_dw.config_2dot4g_dw_band = hidl_request.configParams
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_24GHZ].validDiscoveryWindowIntervalVal;
  legacy_request->config_dw.dw_2dot4g_interval_val = hidl_request.configParams
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_24GHZ].discoveryWindowIntervalVal;
  legacy_request->config_5g_rssi_close = 0; // TODO: don't know default yet
  legacy_request->rssi_close_5g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiClose;
  legacy_request->config_5g_rssi_middle = 0; // TODO: don't know default yet
  legacy_request->rssi_middle_5g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiMiddle;
  legacy_request->config_5g_rssi_close_proximity = 0; // TODO: don't know default yet
  legacy_request->rssi_close_proximity_5g_val =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiProximity;
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
        hidl_request.configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
  legacy_request->config_dw.config_5g_dw_band = hidl_request.configParams
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_5GHZ].validDiscoveryWindowIntervalVal;
  legacy_request->config_dw.dw_5g_interval_val = hidl_request.configParams
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_5GHZ].discoveryWindowIntervalVal;
  if (hidl_request.debugConfigs.validClusterIdVals) {
    legacy_request->cluster_low = hidl_request.debugConfigs.clusterIdLowVal;
    legacy_request->cluster_high = hidl_request.debugConfigs.clusterIdHighVal;
  } else { // need 'else' since not configurable in legacy HAL
    legacy_request->cluster_low = 0x0000;
    legacy_request->cluster_high = 0xFFFF;
  }
  legacy_request->config_intf_addr = hidl_request.debugConfigs.validIntfAddrVal;
  memcpy(legacy_request->intf_addr_val, hidl_request.debugConfigs.intfAddrVal.data(), 6);
  legacy_request->config_oui = hidl_request.debugConfigs.validOuiVal;
  legacy_request->oui_val = hidl_request.debugConfigs.ouiVal;
  legacy_request->config_random_factor_force = hidl_request.debugConfigs.validRandomFactorForceVal;
  legacy_request->random_factor_force_val = hidl_request.debugConfigs.randomFactorForceVal;
  legacy_request->config_hop_count_force = hidl_request.debugConfigs.validHopCountForceVal;
  legacy_request->hop_count_force_val = hidl_request.debugConfigs.hopCountForceVal;
  legacy_request->config_24g_channel = hidl_request.debugConfigs.validDiscoveryChannelVal;
  legacy_request->channel_24g_val =
        hidl_request.debugConfigs.discoveryChannelMhzVal[(size_t) NanBandIndex::NAN_BAND_24GHZ];
  legacy_request->config_5g_channel = hidl_request.debugConfigs.validDiscoveryChannelVal;
  legacy_request->channel_5g_val = hidl_request.debugConfigs
        .discoveryChannelMhzVal[(size_t) NanBandIndex::NAN_BAND_5GHZ];
  legacy_request->config_2dot4g_beacons = hidl_request.debugConfigs.validUseBeaconsInBandVal;
  legacy_request->beacon_2dot4g_val = hidl_request.debugConfigs
        .useBeaconsInBandVal[(size_t) NanBandIndex::NAN_BAND_24GHZ];
  legacy_request->config_5g_beacons = hidl_request.debugConfigs.validUseBeaconsInBandVal;
  legacy_request->beacon_5g_val = hidl_request.debugConfigs
        .useBeaconsInBandVal[(size_t) NanBandIndex::NAN_BAND_5GHZ];
  legacy_request->config_2dot4g_sdf = hidl_request.debugConfigs.validUseSdfInBandVal;
  legacy_request->sdf_2dot4g_val = hidl_request.debugConfigs
        .useSdfInBandVal[(size_t) NanBandIndex::NAN_BAND_24GHZ];
  legacy_request->config_5g_sdf = hidl_request.debugConfigs.validUseSdfInBandVal;
  legacy_request->sdf_5g_val = hidl_request.debugConfigs
        .useSdfInBandVal[(size_t) NanBandIndex::NAN_BAND_5GHZ];

  return true;
}

bool convertHidlNanPublishRequestToLegacy(
    const NanPublishRequest& hidl_request,
    legacy_hal::NanPublishRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanPublishRequest));

  legacy_request->publish_id = hidl_request.baseConfigs.sessionId;
  legacy_request->ttl = hidl_request.baseConfigs.ttlSec;
  legacy_request->period = hidl_request.baseConfigs.discoveryWindowPeriod;
  legacy_request->publish_count = hidl_request.baseConfigs.discoveryCount;
  legacy_request->service_name_len = hidl_request.baseConfigs.serviceName.size();
  if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
    return false;
  }
  memcpy(legacy_request->service_name, hidl_request.baseConfigs.serviceName.c_str(),
        legacy_request->service_name_len);
  legacy_request->publish_match_indicator =
        (legacy_hal::NanMatchAlg) hidl_request.baseConfigs.discoveryMatchIndicator;
  legacy_request->service_specific_info_len = hidl_request.baseConfigs.serviceSpecificInfo.size();
  if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->service_specific_info,
        hidl_request.baseConfigs.serviceSpecificInfo.data(),
        legacy_request->service_specific_info_len);
  legacy_request->rx_match_filter_len = hidl_request.baseConfigs.rxMatchFilter.size();
  if (legacy_request->rx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
    return false;
  }
  memcpy(legacy_request->rx_match_filter,
        hidl_request.baseConfigs.rxMatchFilter.data(),
        legacy_request->rx_match_filter_len);
  legacy_request->tx_match_filter_len = hidl_request.baseConfigs.txMatchFilter.size();
  if (legacy_request->tx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
    return false;
  }
  memcpy(legacy_request->tx_match_filter,
        hidl_request.baseConfigs.txMatchFilter.data(),
        legacy_request->tx_match_filter_len);
  legacy_request->rssi_threshold_flag = hidl_request.baseConfigs.useRssiThreshold;
  legacy_request->recv_indication_cfg = 0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableDiscoveryTerminationIndication ? 0x1 : 0x0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableMatchExpirationIndication ? 0x2 : 0x0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableFollowupReceivedIndication ? 0x4 : 0x0;
  legacy_request->cipher_type = hidl_request.baseConfigs.supportedCipherTypes;
  legacy_request->pmk_len = hidl_request.baseConfigs.pmk.size();
  if (legacy_request->pmk_len > NAN_PMK_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->pmk,
        hidl_request.baseConfigs.pmk.data(),
        legacy_request->pmk_len);
  legacy_request->sdea_params.security_cfg = hidl_request.baseConfigs.securityEnabledInNdp ?
        legacy_hal::NAN_DP_CONFIG_SECURITY : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
  legacy_request->sdea_params.ranging_state = hidl_request.baseConfigs.rangingRequired ?
        legacy_hal::NAN_RANGING_ENABLE : legacy_hal::NAN_RANGING_DISABLE;
  legacy_request->ranging_cfg.ranging_interval_msec = hidl_request.baseConfigs.rangingIntervalMsec;
  legacy_request->ranging_cfg.config_ranging_indications =
        hidl_request.baseConfigs.configRangingIndications;
  legacy_request->ranging_cfg.distance_ingress_cm = hidl_request.baseConfigs.distanceIngressCm;
  legacy_request->ranging_cfg.distance_egress_cm = hidl_request.baseConfigs.distanceEgressCm;
  legacy_request->publish_type = (legacy_hal::NanPublishType) hidl_request.publishType;
  legacy_request->tx_type = (legacy_hal::NanTxType) hidl_request.txType;

  return true;
}

bool convertHidlNanSubscribeRequestToLegacy(
    const NanSubscribeRequest& hidl_request,
    legacy_hal::NanSubscribeRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanSubscribeRequest));

  legacy_request->subscribe_id = hidl_request.baseConfigs.sessionId;
  legacy_request->ttl = hidl_request.baseConfigs.ttlSec;
  legacy_request->period = hidl_request.baseConfigs.discoveryWindowPeriod;
  legacy_request->subscribe_count = hidl_request.baseConfigs.discoveryCount;
  legacy_request->service_name_len = hidl_request.baseConfigs.serviceName.size();
  if (legacy_request->service_name_len > NAN_MAX_SERVICE_NAME_LEN) {
    return false;
  }
  memcpy(legacy_request->service_name, hidl_request.baseConfigs.serviceName.c_str(),
        legacy_request->service_name_len);
  legacy_request->subscribe_match_indicator =
        (legacy_hal::NanMatchAlg) hidl_request.baseConfigs.discoveryMatchIndicator;
  legacy_request->service_specific_info_len = hidl_request.baseConfigs.serviceSpecificInfo.size();
  if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->service_specific_info,
        hidl_request.baseConfigs.serviceSpecificInfo.data(),
        legacy_request->service_specific_info_len);
  legacy_request->rx_match_filter_len = hidl_request.baseConfigs.rxMatchFilter.size();
  if (legacy_request->rx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
    return false;
  }
  memcpy(legacy_request->rx_match_filter,
        hidl_request.baseConfigs.rxMatchFilter.data(),
        legacy_request->rx_match_filter_len);
  legacy_request->tx_match_filter_len = hidl_request.baseConfigs.txMatchFilter.size();
  if (legacy_request->tx_match_filter_len > NAN_MAX_MATCH_FILTER_LEN) {
    return false;
  }
  memcpy(legacy_request->tx_match_filter,
        hidl_request.baseConfigs.txMatchFilter.data(),
        legacy_request->tx_match_filter_len);
  legacy_request->rssi_threshold_flag = hidl_request.baseConfigs.useRssiThreshold;
  legacy_request->recv_indication_cfg = 0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableDiscoveryTerminationIndication ? 0x1 : 0x0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableMatchExpirationIndication ? 0x2 : 0x0;
  legacy_request->recv_indication_cfg |=
        hidl_request.baseConfigs.disableFollowupReceivedIndication ? 0x4 : 0x0;
  legacy_request->cipher_type = hidl_request.baseConfigs.supportedCipherTypes;
  legacy_request->pmk_len = hidl_request.baseConfigs.pmk.size();
  if (legacy_request->pmk_len > NAN_PMK_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->pmk,
        hidl_request.baseConfigs.pmk.data(),
        legacy_request->pmk_len);
  legacy_request->sdea_params.security_cfg = hidl_request.baseConfigs.securityEnabledInNdp ?
        legacy_hal::NAN_DP_CONFIG_SECURITY : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
  legacy_request->sdea_params.ranging_state = hidl_request.baseConfigs.rangingRequired ?
        legacy_hal::NAN_RANGING_ENABLE : legacy_hal::NAN_RANGING_DISABLE;
  legacy_request->ranging_cfg.ranging_interval_msec = hidl_request.baseConfigs.rangingIntervalMsec;
  legacy_request->ranging_cfg.config_ranging_indications =
        hidl_request.baseConfigs.configRangingIndications;
  legacy_request->ranging_cfg.distance_ingress_cm = hidl_request.baseConfigs.distanceIngressCm;
  legacy_request->ranging_cfg.distance_egress_cm = hidl_request.baseConfigs.distanceEgressCm;
  legacy_request->subscribe_type = (legacy_hal::NanSubscribeType) hidl_request.subscribeType;
  legacy_request->serviceResponseFilter = (legacy_hal::NanSRFType) hidl_request.srfType;
  legacy_request->serviceResponseInclude = hidl_request.srfRespondIfInAddressSet ?
        legacy_hal::NAN_SRF_INCLUDE_RESPOND : legacy_hal::NAN_SRF_INCLUDE_DO_NOT_RESPOND;
  legacy_request->useServiceResponseFilter = hidl_request.shouldUseSrf ?
        legacy_hal::NAN_USE_SRF : legacy_hal::NAN_DO_NOT_USE_SRF;
  legacy_request->ssiRequiredForMatchIndication = hidl_request.isSsiRequiredForMatch ?
        legacy_hal::NAN_SSI_REQUIRED_IN_MATCH_IND : legacy_hal::NAN_SSI_NOT_REQUIRED_IN_MATCH_IND;
  legacy_request->num_intf_addr_present = hidl_request.intfAddr.size();
  if (legacy_request->num_intf_addr_present > NAN_MAX_SUBSCRIBE_MAX_ADDRESS) {
    return false;
  }
  for (int i = 0; i < legacy_request->num_intf_addr_present; i++) {
    memcpy(legacy_request->intf_addr[i], hidl_request.intfAddr[i].data(), 6);
  }

  return true;
}

bool convertHidlNanTransmitFollowupRequestToLegacy(
    const NanTransmitFollowupRequest& hidl_request,
    legacy_hal::NanTransmitFollowupRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanTransmitFollowupRequest));

  legacy_request->publish_subscribe_id = hidl_request.discoverySessionId;
  legacy_request->requestor_instance_id = hidl_request.peerId;
  memcpy(legacy_request->addr, hidl_request.addr.data(), 6);
  legacy_request->priority = hidl_request.isHighPriority ?
        legacy_hal::NAN_TX_PRIORITY_HIGH : legacy_hal::NAN_TX_PRIORITY_NORMAL;
  legacy_request->dw_or_faw = hidl_request.shouldUseDiscoveryWindow ?
        legacy_hal::NAN_TRANSMIT_IN_DW : legacy_hal::NAN_TRANSMIT_IN_FAW;
  legacy_request->service_specific_info_len = hidl_request.message.size();
  if (legacy_request->service_specific_info_len > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->service_specific_info,
        hidl_request.message.data(),
        legacy_request->service_specific_info_len);
  legacy_request->recv_indication_cfg = hidl_request.disableFollowupResultIndication ? 0x1 : 0x0;

  return true;
}

bool convertHidlNanConfigRequestToLegacy(
    const NanConfigRequest& hidl_request,
    legacy_hal::NanConfigRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanConfigRequest));

  // TODO: b/34059183 tracks missing configurations in legacy HAL or uknown defaults
  legacy_request->master_pref = hidl_request.masterPref;
  legacy_request->discovery_indication_cfg = 0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.disableDiscoveryAddressChangeIndication ? 0x1 : 0x0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.disableStartedClusterIndication ? 0x2 : 0x0;
  legacy_request->discovery_indication_cfg |=
        hidl_request.disableJoinedClusterIndication ? 0x4 : 0x0;
  legacy_request->config_sid_beacon = 1;
  if (hidl_request.numberOfServiceIdsInBeacon > 127) {
    return false;
  }
  legacy_request->sid_beacon = (hidl_request.includeServiceIdsInBeacon ? 0x1 : 0x0)
        | (hidl_request.numberOfServiceIdsInBeacon << 1);
  legacy_request->config_rssi_window_size = 0; // TODO: don't know default yet
  legacy_request->rssi_window_size_val = hidl_request.rssiWindowSize;
  legacy_request->config_disc_mac_addr_randomization = 1;
  legacy_request->disc_mac_addr_rand_interval_sec =
        hidl_request.macAddressRandomizationIntervalSec;
  legacy_request->config_responder_auto_response = 1;
  legacy_request->ranging_auto_response_cfg = hidl_request.acceptRangingRequests ?
       legacy_hal::NAN_RANGING_AUTO_RESPONSE_ENABLE : legacy_hal::NAN_RANGING_AUTO_RESPONSE_DISABLE;
  /* TODO : missing
  legacy_request->config_2dot4g_rssi_close = 0; // TODO: don't know default yet
  legacy_request->rssi_close_2dot4g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiClose;
  legacy_request->config_2dot4g_rssi_middle = 0; // TODO: don't know default yet
  legacy_request->rssi_middle_2dot4g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiMiddle;
  legacy_request->config_2dot4g_rssi_proximity = 0; // TODO: don't know default yet
  legacy_request->rssi_proximity_2dot4g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].rssiProximity;
  */
  legacy_request->config_scan_params = 0; // TODO: don't know default yet
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_24G_BAND] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_24G_BAND] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ].scanPeriodSec;
  legacy_request->config_dw.config_2dot4g_dw_band = hidl_request
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_24GHZ].validDiscoveryWindowIntervalVal;
  legacy_request->config_dw.dw_2dot4g_interval_val = hidl_request
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_24GHZ].discoveryWindowIntervalVal;
  /* TODO: missing
  legacy_request->config_5g_rssi_close = 0; // TODO: don't know default yet
  legacy_request->rssi_close_5g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiClose;
  legacy_request->config_5g_rssi_middle = 0; // TODO: don't know default yet
  legacy_request->rssi_middle_5g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiMiddle;
  */
  legacy_request->config_5g_rssi_close_proximity = 0; // TODO: don't know default yet
  legacy_request->rssi_close_proximity_5g_val =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].rssiProximity;
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_LOW] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
  legacy_request->scan_params_val.dwell_time[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].dwellTimeMs;
  legacy_request->scan_params_val.scan_period[legacy_hal::NAN_CHANNEL_5G_BAND_HIGH] =
        hidl_request.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ].scanPeriodSec;
  legacy_request->config_dw.config_5g_dw_band = hidl_request
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_5GHZ].validDiscoveryWindowIntervalVal;
  legacy_request->config_dw.dw_5g_interval_val = hidl_request
        .bandSpecificConfig[(size_t) NanBandIndex::NAN_BAND_5GHZ].discoveryWindowIntervalVal;

  return true;
}

bool convertHidlNanBeaconSdfPayloadRequestToLegacy(
    const NanBeaconSdfPayloadRequest& hidl_request,
    legacy_hal::NanBeaconSdfPayloadRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanBeaconSdfPayloadRequest));

  legacy_request->vsa.payload_transmit_flag = hidl_request.transmitInNext16dws ? 1 : 0;
  legacy_request->vsa.tx_in_discovery_beacon = hidl_request.transmitInDiscoveryBeacon;
  legacy_request->vsa.tx_in_sync_beacon = hidl_request.transmitInSyncBeacon;
  legacy_request->vsa.tx_in_service_discovery = hidl_request.transmitInServiceDiscoveryFrame;
  legacy_request->vsa.vendor_oui = hidl_request.vendorOui;
  legacy_request->vsa.vsa_len = hidl_request.vsa.size();
  if (legacy_request->vsa.vsa_len > NAN_MAX_VSA_DATA_LEN) {
    return false;
  }
  memcpy(legacy_request->vsa.vsa, hidl_request.vsa.data(), legacy_request->vsa.vsa_len);

  return true;
}

bool convertHidlNanDataPathInitiatorRequestToLegacy(
    const NanInitiateDataPathRequest& hidl_request,
    legacy_hal::NanDataPathInitiatorRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanDataPathInitiatorRequest));

  legacy_request->requestor_instance_id = hidl_request.peerId;
  memcpy(legacy_request->peer_disc_mac_addr, hidl_request.peerDiscMacAddr.data(), 6);
  legacy_request->channel_request_type =
        (legacy_hal::NanDataPathChannelCfg) hidl_request.channelRequestType;
  legacy_request->channel = hidl_request.channel;
  strcpy(legacy_request->ndp_iface, hidl_request.ifaceName.c_str());
  legacy_request->ndp_cfg.security_cfg = hidl_request.securityRequired ?
        legacy_hal::NAN_DP_CONFIG_SECURITY : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
  legacy_request->app_info.ndp_app_info_len = hidl_request.appInfo.size();
  if (legacy_request->app_info.ndp_app_info_len > NAN_DP_MAX_APP_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->app_info.ndp_app_info, hidl_request.appInfo.data(),
        legacy_request->app_info.ndp_app_info_len);
  legacy_request->cipher_type = hidl_request.supportedCipherTypes;
  legacy_request->pmk_len = hidl_request.pmk.size();
  if (legacy_request->pmk_len > NAN_PMK_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->pmk, hidl_request.pmk.data(), legacy_request->pmk_len);

  return true;
}

bool convertHidlNanDataPathIndicationResponseToLegacy(
    const NanRespondToDataPathIndicationRequest& hidl_request,
    legacy_hal::NanDataPathIndicationResponse* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  memset(legacy_request, 0, sizeof(legacy_hal::NanDataPathIndicationResponse));

  legacy_request->rsp_code = hidl_request.acceptRequest ?
        legacy_hal::NAN_DP_REQUEST_ACCEPT : legacy_hal::NAN_DP_REQUEST_REJECT;
  legacy_request->ndp_instance_id = hidl_request.ndpInstanceId;
  strcpy(legacy_request->ndp_iface, hidl_request.ifaceName.c_str());
  legacy_request->ndp_cfg.security_cfg = hidl_request.securityRequired ?
        legacy_hal::NAN_DP_CONFIG_SECURITY : legacy_hal::NAN_DP_CONFIG_NO_SECURITY;
  legacy_request->app_info.ndp_app_info_len = hidl_request.appInfo.size();
  if (legacy_request->app_info.ndp_app_info_len > NAN_DP_MAX_APP_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->app_info.ndp_app_info, hidl_request.appInfo.data(),
        legacy_request->app_info.ndp_app_info_len);
  legacy_request->cipher_type = hidl_request.supportedCipherTypes;
  legacy_request->pmk_len = hidl_request.pmk.size();
  if (legacy_request->pmk_len > NAN_PMK_INFO_LEN) {
    return false;
  }
  memcpy(legacy_request->pmk, hidl_request.pmk.data(), legacy_request->pmk_len);

  return true;
}

bool convertLegacyNanResponseHeaderToHidl(
    const legacy_hal::NanResponseMsg& legacy_response,
    WifiNanStatus* wifiNanStatus) {
  if (!wifiNanStatus) {
    return false;
  }
  wifiNanStatus->status = convertLegacyNanStatusTypeToHidl(legacy_response.status);
  wifiNanStatus->description = legacy_response.nan_error;

  return true;
}

bool convertLegacyNanCapabilitiesResponseToHidl(
    const legacy_hal::NanCapabilities& legacy_response,
    NanCapabilities* hidl_response) {
  if (!hidl_response) {
    return false;
  }
  hidl_response->maxConcurrentClusters = legacy_response.max_concurrent_nan_clusters;
  hidl_response->maxPublishes = legacy_response.max_publishes;
  hidl_response->maxSubscribes = legacy_response.max_subscribes;
  hidl_response->maxServiceNameLen = legacy_response.max_service_name_len;
  hidl_response->maxMatchFilterLen = legacy_response.max_match_filter_len;
  hidl_response->maxTotalMatchFilterLen = legacy_response.max_total_match_filter_len;
  hidl_response->maxServiceSpecificInfoLen = legacy_response.max_service_specific_info_len;
  hidl_response->maxVsaDataLen = legacy_response.max_vsa_data_len;
  hidl_response->maxNdiInterfaces = legacy_response.max_ndi_interfaces;
  hidl_response->maxNdpSessions = legacy_response.max_ndp_sessions;
  hidl_response->maxAppInfoLen = legacy_response.max_app_info_len;
  hidl_response->maxQueuedTransmitFollowupMsgs = legacy_response.max_queued_transmit_followup_msgs;
  // TODO: b/34059183 to add to underlying HAL
  hidl_response->maxSubscribeInterfaceAddresses = NAN_MAX_SUBSCRIBE_MAX_ADDRESS;
  hidl_response->supportedCipherSuites = legacy_response.cipher_suites_supported;

  return true;
}

bool convertLegacyNanMatchIndToHidl(
    const legacy_hal::NanMatchInd& legacy_ind,
    NanMatchInd* hidl_ind) {
  if (!hidl_ind) {
    return false;
  }
  hidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
  hidl_ind->peerId = legacy_ind.requestor_instance_id;
  hidl_ind->addr = hidl_array<uint8_t, 6>(legacy_ind.addr);
  hidl_ind->serviceSpecificInfo = std::vector<uint8_t>(legacy_ind.service_specific_info,
        legacy_ind.service_specific_info + legacy_ind.service_specific_info_len);
  hidl_ind->matchFilter = std::vector<uint8_t>(legacy_ind.sdf_match_filter,
        legacy_ind.sdf_match_filter + legacy_ind.sdf_match_filter_len);
  hidl_ind->matchOccuredInBeaconFlag = legacy_ind.match_occured_flag == 1;
  hidl_ind->outOfResourceFlag = legacy_ind.out_of_resource_flag == 1;
  hidl_ind->rssiValue = legacy_ind.rssi_value;
  hidl_ind->peerSupportedCipherTypes = legacy_ind.peer_cipher_type;
  hidl_ind->peerRequiresSecurityEnabledInNdp =
        legacy_ind.peer_sdea_params.security_cfg == legacy_hal::NAN_DP_CONFIG_SECURITY;
  hidl_ind->peerRequiresRanging =
        legacy_ind.peer_sdea_params.ranging_state == legacy_hal::NAN_RANGING_ENABLE;
  hidl_ind->rangingMeasurementInCm = legacy_ind.range_result.range_measurement_cm;
  hidl_ind->rangingIndicationType = legacy_ind.range_result.ranging_event_type;

  return true;
}

bool convertLegacyNanFollowupIndToHidl(
    const legacy_hal::NanFollowupInd& legacy_ind,
    NanFollowupReceivedInd* hidl_ind) {
  if (!hidl_ind) {
    return false;
  }
  hidl_ind->discoverySessionId = legacy_ind.publish_subscribe_id;
  hidl_ind->peerId = legacy_ind.requestor_instance_id;
  hidl_ind->addr = hidl_array<uint8_t, 6>(legacy_ind.addr);
  hidl_ind->receivedInFaw = legacy_ind.dw_or_faw == 1;
  hidl_ind->message = std::vector<uint8_t>(legacy_ind.service_specific_info,
        legacy_ind.service_specific_info + legacy_ind.service_specific_info_len);

  return true;
}

bool convertLegacyNanBeaconSdfPayloadIndToHidl(
    const legacy_hal::NanBeaconSdfPayloadInd& legacy_ind,
    NanBeaconSdfPayloadInd* hidl_ind) {
  if (!hidl_ind) {
    return false;
  }
  hidl_ind->addr = hidl_array<uint8_t, 6>(legacy_ind.addr);
  hidl_ind->isVsaReceived = legacy_ind.is_vsa_received == 1;
  hidl_ind->vsaReceivedOnFrames = legacy_ind.vsa.vsa_received_on;
  hidl_ind->vsaVendorOui = legacy_ind.vsa.vendor_oui;
  hidl_ind->vsa = std::vector<uint8_t>(legacy_ind.vsa.vsa,
        legacy_ind.vsa.vsa + legacy_ind.vsa.attr_len);
  hidl_ind->isBeaconSdfPayloadReceived = legacy_ind.is_beacon_sdf_payload_received == 1;
  hidl_ind->beaconSdfPayloadData = std::vector<uint8_t>(legacy_ind.data.frame_data,
        legacy_ind.data.frame_data + legacy_ind.data.frame_len);

  return true;
}

bool convertLegacyNanDataPathRequestIndToHidl(
    const legacy_hal::NanDataPathRequestInd& legacy_ind,
    NanDataPathRequestInd* hidl_ind) {
  if (!hidl_ind) {
    return false;
  }
  hidl_ind->discoverySessionId = legacy_ind.service_instance_id;
  hidl_ind->peerDiscMacAddr = hidl_array<uint8_t, 6>(legacy_ind.peer_disc_mac_addr);
  hidl_ind->ndpInstanceId = legacy_ind.ndp_instance_id;
  hidl_ind->securityRequired =
        legacy_ind.ndp_cfg.security_cfg == legacy_hal::NAN_DP_CONFIG_SECURITY;
  hidl_ind->appInfo = std::vector<uint8_t>(legacy_ind.app_info.ndp_app_info,
        legacy_ind.app_info.ndp_app_info + legacy_ind.app_info.ndp_app_info_len);

  return true;
}

bool convertLegacyNanDataPathConfirmIndToHidl(
    const legacy_hal::NanDataPathConfirmInd& legacy_ind,
    NanDataPathConfirmInd* hidl_ind) {
  if (!hidl_ind) {
    return false;
  }
  hidl_ind->ndpInstanceId = legacy_ind.ndp_instance_id;
  hidl_ind->dataPathSetupSuccess = legacy_ind.rsp_code == legacy_hal::NAN_DP_REQUEST_ACCEPT;
  hidl_ind->peerNdiMacAddr = hidl_array<uint8_t, 6>(legacy_ind.peer_ndi_mac_addr);
  hidl_ind->appInfo = std::vector<uint8_t>(legacy_ind.app_info.ndp_app_info,
          legacy_ind.app_info.ndp_app_info + legacy_ind.app_info.ndp_app_info_len);
  hidl_ind->status.status = convertLegacyNanStatusTypeToHidl(legacy_ind.reason_code);
  hidl_ind->status.description = ""; // TODO: b/34059183

  return true;
}

}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace hardware
}  // namespace android
