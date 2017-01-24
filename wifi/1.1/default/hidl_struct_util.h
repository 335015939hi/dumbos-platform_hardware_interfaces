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

#ifndef HIDL_STRUCT_UTIL_1_1_H_
#define HIDL_STRUCT_UTIL_1_1_H_

#include <vector>

#include <android/hardware/wifi/1.1/types.h>

#include "../../1.0/default/wifi_legacy_hal.h"

/**
 * This file contains a bunch of functions to convert structs from the legacy
 * HAL to HIDL and vice versa.
 * TODO(b/32093047): Add unit tests for these conversion methods in the VTS test
 * suite.
 */
namespace android {
namespace hardware {
namespace wifi {
namespace V1_1 {
namespace implementation {
namespace hidl_struct_util {

using namespace android::hardware::wifi::V1_0::implementation;

// NAN iface conversion methods.
NanStatusType convertLegacyNanStatusTypeToHidl(legacy_hal::NanStatusType type);
bool convertHidlNanEnableRequestToLegacy(
    const NanEnableRequest& hidl_request,
    legacy_hal::NanEnableRequest* legacy_request);
bool convertHidlNanConfigRequestToLegacy(
    const NanConfigRequest& hidl_request,
    legacy_hal::NanConfigRequest* legacy_request);
bool convertHidlNanPublishRequestToLegacy(
    const NanPublishRequest& hidl_request,
    legacy_hal::NanPublishRequest* legacy_request);
bool convertHidlNanSubscribeRequestToLegacy(
    const NanSubscribeRequest& hidl_request,
    legacy_hal::NanSubscribeRequest* legacy_request);
bool convertHidlNanTransmitFollowupRequestToLegacy(
    const NanTransmitFollowupRequest& hidl_request,
    legacy_hal::NanTransmitFollowupRequest* legacy_request);
bool convertHidlNanBeaconSdfPayloadRequestToLegacy(
    const NanBeaconSdfPayloadRequest& hidl_request,
    legacy_hal::NanBeaconSdfPayloadRequest* legacy_request);
bool convertHidlNanDataPathInitiatorRequestToLegacy(
    const NanInitiateDataPathRequest& hidl_request,
    legacy_hal::NanDataPathInitiatorRequest* legacy_request);
bool convertHidlNanDataPathIndicationResponseToLegacy(
    const NanRespondToDataPathIndicationRequest& hidl_response,
    legacy_hal::NanDataPathIndicationResponse* legacy_response);
bool convertLegacyNanResponseHeaderToHidl(
    const legacy_hal::NanResponseMsg& legacy_response,
    WifiNanStatus* wifiNanStatus);
bool convertLegacyNanCapabilitiesResponseToHidl(
    const legacy_hal::NanCapabilities& legacy_response,
    NanCapabilities* hidl_response);
bool convertLegacyNanMatchIndToHidl(const legacy_hal::NanMatchInd& legacy_ind,
                                    NanMatchInd* hidl_ind);
bool convertLegacyNanFollowupIndToHidl(
    const legacy_hal::NanFollowupInd& legacy_ind, NanFollowupReceivedInd* hidl_ind);
bool convertLegacyNanDataPathRequestIndToHidl(
    const legacy_hal::NanDataPathRequestInd& legacy_ind,
    NanDataPathRequestInd* hidl_ind);
bool convertLegacyNanDataPathConfirmIndToHidl(
    const legacy_hal::NanDataPathConfirmInd& legacy_ind,
    NanDataPathConfirmInd* hidl_ind);
bool convertLegacyNanBeaconSdfPayloadIndToHidl(
    const legacy_hal::NanBeaconSdfPayloadInd& legacy_ind,
    NanBeaconSdfPayloadInd* hidl_ind);

}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_1_1_H_
