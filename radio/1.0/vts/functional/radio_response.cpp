/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <radio_hidl_hal_utils.h>

CardStatus cardStatus;

Return<void> RadioResponse::getIccCardStatusResponse(
    const RadioResponseInfo& info, const CardStatus& card_status) {
  cardStatus = card_status;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::supplyIccPinForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::supplyIccPukForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::supplyIccPin2ForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::supplyIccPuk2ForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::changeIccPinForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::changeIccPin2ForAppResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::supplyNetworkDepersonalizationResponse(
    const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCurrentCallsResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<Call>& /*calls*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::dialResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getIMSIForAppResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& imsi) {
  this->imsi = imsi;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::hangupConnectionResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::hangupWaitingOrBackgroundResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::hangupForegroundResumeBackgroundResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::switchWaitingOrHoldingAndActiveResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::conferenceResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::rejectCallResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getLastCallFailCauseResponse(
    const RadioResponseInfo& info,
    const LastCallFailCauseInfo& /*failCauseInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getSignalStrengthResponse(
    const RadioResponseInfo& info, const SignalStrength& /*sig_strength*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse(
    const RadioResponseInfo& info,
    const VoiceRegStateResult& /*voiceRegResponse*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getDataRegistrationStateResponse(
    const RadioResponseInfo& info,
    const DataRegStateResult& /*dataRegResponse*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getOperatorResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*longName*/,
    const ::android::hardware::hidl_string& /*shortName*/,
    const ::android::hardware::hidl_string& /*numeric*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setRadioPowerResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendDtmfResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendSmsResponse(const RadioResponseInfo& info,
                                            const SendSmsResult& sms) {
  sendSmsResult = sms;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendSMSExpectMoreResponse(
    const RadioResponseInfo& info, const SendSmsResult& sms) {
  sendSmsResult = sms;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setupDataCallResponse(
    const RadioResponseInfo& info, const SetupDataCallResult& /*dcResponse*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::iccIOForAppResponse(const RadioResponseInfo& info,
                                                const IccIoResult& iccIo) {
  this->iccIoResult = iccIo;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendUssdResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::cancelPendingUssdResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getClirResponse(const RadioResponseInfo& info,
                                            int32_t /*n*/, int32_t /*m*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setClirResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCallForwardStatusResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<CallForwardInfo>&
    /*callForwardInfos*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCallForwardResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCallWaitingResponse(
    const RadioResponseInfo& info, bool /*enable*/, int32_t /*serviceClass*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCallWaitingResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::acknowledgeLastIncomingGsmSmsResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::acceptCallResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::deactivateDataCallResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getFacilityLockForAppResponse(
    const RadioResponseInfo& info, int32_t /*response*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setFacilityLockForAppResponse(
    const RadioResponseInfo& info, int32_t /*retry*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setBarringPasswordResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getNetworkSelectionModeResponse(
    const RadioResponseInfo& info, bool /*manual*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setNetworkSelectionModeAutomaticResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getAvailableNetworksResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::startDtmfResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::stopDtmfResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getBasebandVersionResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*version*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::separateConnectionResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setMuteResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getMuteResponse(const RadioResponseInfo& info,
                                            bool /*enable*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getClipResponse(const RadioResponseInfo& info,
                                            ClipStatus /*status*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getDataCallListResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<SetupDataCallResult>& /*dcResponse*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendOemRilRequestRawResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
  return Void();
}

Return<void> RadioResponse::sendOemRilRequestStringsResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<
        ::android::hardware::hidl_string>& /*data*/) {
  return Void();
}

Return<void> RadioResponse::setSuppServiceNotificationsResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::writeSmsToSimResponse(const RadioResponseInfo& info,
                                                  int32_t index) {
  writeSmsToSimIndex = index;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::deleteSmsOnSimResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setBandModeResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getAvailableBandModesResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<RadioBandMode>& /*bandModes*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendEnvelopeResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*commandResponse*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendTerminalResponseToSimResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::handleStkCallSetupRequestFromSimResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::explicitCallTransferResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setPreferredNetworkTypeResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getPreferredNetworkTypeResponse(
    const RadioResponseInfo& info, PreferredNetworkType /*nw_type*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getNeighboringCidsResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setLocationUpdatesResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCdmaSubscriptionSourceResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCdmaRoamingPreferenceResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCdmaRoamingPreferenceResponse(
    const RadioResponseInfo& info, CdmaRoamingType /*type*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setTTYModeResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getTTYModeResponse(const RadioResponseInfo& info,
                                               TtyMode /*mode*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setPreferredVoicePrivacyResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getPreferredVoicePrivacyResponse(
    const RadioResponseInfo& info, bool /*enable*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendCDMAFeatureCodeResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendBurstDtmfResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendCdmaSmsResponse(const RadioResponseInfo& info,
                                                const SendSmsResult& sms) {
  sendSmsResult = sms;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::acknowledgeLastIncomingCdmaSmsResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getGsmBroadcastConfigResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<
        GsmBroadcastSmsConfigInfo>& /*configs*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setGsmBroadcastConfigResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setGsmBroadcastActivationResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCdmaBroadcastConfigResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<
        CdmaBroadcastSmsConfigInfo>& /*configs*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCdmaBroadcastConfigResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCdmaBroadcastActivationResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCDMASubscriptionResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*mdn*/,
    const ::android::hardware::hidl_string& /*hSid*/,
    const ::android::hardware::hidl_string& /*hNid*/,
    const ::android::hardware::hidl_string& /*min*/,
    const ::android::hardware::hidl_string& /*prl*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::writeSmsToRuimResponse(
    const RadioResponseInfo& info, uint32_t index) {
  writeSmsToRuimIndex = index;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::deleteSmsOnRuimResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getDeviceIdentityResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*imei*/,
    const ::android::hardware::hidl_string& /*imeisv*/,
    const ::android::hardware::hidl_string& /*esn*/,
    const ::android::hardware::hidl_string& /*meid*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::exitEmergencyCallbackModeResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getSmscAddressResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& smsc) {
  smscAddress = smsc;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setSmscAddressResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::reportSmsMemoryStatusResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::reportStkServiceIsRunningResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCdmaSubscriptionSourceResponse(
    const RadioResponseInfo& info, CdmaSubscriptionSource /*source*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::requestIsimAuthenticationResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*response*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::acknowledgeIncomingGsmSmsWithPduResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendEnvelopeWithStatusResponse(
    const RadioResponseInfo& info, const IccIoResult& /*iccIo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getVoiceRadioTechnologyResponse(
    const RadioResponseInfo& info, RadioTechnology /*rat*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getCellInfoListResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<CellInfo>& /*cellInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setCellInfoListRateResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setInitialAttachApnResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getImsRegistrationStateResponse(
    const RadioResponseInfo& info, bool /*isRegistered*/,
    RadioTechnologyFamily /*ratFamily*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendImsSmsResponse(const RadioResponseInfo& info,
                                               const SendSmsResult& sms) {
  sendSmsResult = sms;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::iccTransmitApduBasicChannelResponse(
    const RadioResponseInfo& info, const IccIoResult& result) {
  this->iccIoResult = result;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::iccOpenLogicalChannelResponse(
    const RadioResponseInfo& info, int32_t channelId,
    const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
  this->channelId = channelId;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::iccCloseLogicalChannelResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::iccTransmitApduLogicalChannelResponse(
    const RadioResponseInfo& info, const IccIoResult& result) {
  this->iccIoResult = result;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::nvReadItemResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_string& /*result*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::nvWriteItemResponse(const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::nvWriteCdmaPrlResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::nvResetConfigResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setUiccSubscriptionResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setDataAllowedResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getHardwareConfigResponse(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::requestIccSimAuthenticationResponse(
    const RadioResponseInfo& info, const IccIoResult& result) {
  this->iccIoResult = result;
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setDataProfileResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::requestShutdownResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getRadioCapabilityResponse(
    const RadioResponseInfo& info, const RadioCapability& /*rc*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setRadioCapabilityResponse(
    const RadioResponseInfo& info, const RadioCapability& /*rc*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::startLceServiceResponse(
    const RadioResponseInfo& info, const LceStatusInfo& /*statusInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::stopLceServiceResponse(
    const RadioResponseInfo& info, const LceStatusInfo& /*statusInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::pullLceDataResponse(
    const RadioResponseInfo& info, const LceDataInfo& /*lceInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getModemActivityInfoResponse(
    const RadioResponseInfo& info, const ActivityStatsInfo& /*activityInfo*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setAllowedCarriersResponse(
    const RadioResponseInfo& info, int32_t /*numAllowed*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::getAllowedCarriersResponse(
    const RadioResponseInfo& info, bool /*allAllowed*/,
    const CarrierRestrictions& /*carriers*/) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::sendDeviceStateResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setIndicationFilterResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::setSimCardPowerResponse(
    const RadioResponseInfo& info) {
  NotifyFromCallback(info);
  return Void();
}

Return<void> RadioResponse::acknowledgeRequest(int32_t /*serial*/) {
  return Void();
}
