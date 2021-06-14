/*
 * Copyright (C) 2021 The Android Open Source Project
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

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include <limits>
#include "android/hardware/radio/1.0/types.h"
#include "android/hardware/radio/1.1/types.h"
#include "android/hardware/radio/1.2/types.h"
#include "android/hardware/radio/1.3/types.h"
#include "android/hardware/radio/1.4/types.h"
#include "android/hardware/radio/1.5/types.h"
#include "android/hardware/radio/1.6/types.h"
#include "android/hardware/radio/AccessNetwork.h"
#include "android/hardware/radio/ActivityStatsInfo.h"
#include "android/hardware/radio/AddressProperty.h"
#include "android/hardware/radio/ApnAuthType.h"
#include "android/hardware/radio/ApnTypes.h"
#include "android/hardware/radio/AppState.h"
#include "android/hardware/radio/AppStatus.h"
#include "android/hardware/radio/AppType.h"
#include "android/hardware/radio/AudioQuality.h"
#include "android/hardware/radio/BarringInfo.h"
#include "android/hardware/radio/BarringInfoBarringType.h"
#include "android/hardware/radio/BarringInfoBarringTypeSpecificInfo.h"
#include "android/hardware/radio/BarringInfoBarringTypeSpecificInfoConditional.h"
#include "android/hardware/radio/BarringInfoServiceType.h"
#include "android/hardware/radio/Call.h"
#include "android/hardware/radio/CallForwardInfo.h"
#include "android/hardware/radio/CallForwardInfoStatus.h"
#include "android/hardware/radio/CallPresentation.h"
#include "android/hardware/radio/CallState.h"
#include "android/hardware/radio/CardPowerState.h"
#include "android/hardware/radio/CardState.h"
#include "android/hardware/radio/CardStatus.h"
#include "android/hardware/radio/Carrier.h"
#include "android/hardware/radio/CarrierMatchType.h"
#include "android/hardware/radio/CarrierRestrictions.h"
#include "android/hardware/radio/CarrierRestrictionsWithPriority.h"
#include "android/hardware/radio/CdmaBroadcastSmsConfigInfo.h"
#include "android/hardware/radio/CdmaCallWaiting.h"
#include "android/hardware/radio/CdmaCallWaitingNumberPlan.h"
#include "android/hardware/radio/CdmaCallWaitingNumberPresentation.h"
#include "android/hardware/radio/CdmaCallWaitingNumberType.h"
#include "android/hardware/radio/CdmaDisplayInfoRecord.h"
#include "android/hardware/radio/CdmaInfoRecName.h"
#include "android/hardware/radio/CdmaInformationRecord.h"
#include "android/hardware/radio/CdmaInformationRecords.h"
#include "android/hardware/radio/CdmaLineControlInfoRecord.h"
#include "android/hardware/radio/CdmaNumberInfoRecord.h"
#include "android/hardware/radio/CdmaOtaProvisionStatus.h"
#include "android/hardware/radio/CdmaRedirectingNumberInfoRecord.h"
#include "android/hardware/radio/CdmaRedirectingReason.h"
#include "android/hardware/radio/CdmaRoamingType.h"
#include "android/hardware/radio/CdmaSignalInfoRecord.h"
#include "android/hardware/radio/CdmaSignalStrength.h"
#include "android/hardware/radio/CdmaSmsAck.h"
#include "android/hardware/radio/CdmaSmsAddress.h"
#include "android/hardware/radio/CdmaSmsDigitMode.h"
#include "android/hardware/radio/CdmaSmsErrorClass.h"
#include "android/hardware/radio/CdmaSmsMessage.h"
#include "android/hardware/radio/CdmaSmsNumberMode.h"
#include "android/hardware/radio/CdmaSmsNumberPlan.h"
#include "android/hardware/radio/CdmaSmsNumberType.h"
#include "android/hardware/radio/CdmaSmsSubaddress.h"
#include "android/hardware/radio/CdmaSmsSubaddressType.h"
#include "android/hardware/radio/CdmaSmsWriteArgs.h"
#include "android/hardware/radio/CdmaSmsWriteArgsStatus.h"
#include "android/hardware/radio/CdmaSubscriptionSource.h"
#include "android/hardware/radio/CdmaT53AudioControlInfoRecord.h"
#include "android/hardware/radio/CdmaT53ClirInfoRecord.h"
#include "android/hardware/radio/CellConfigLte.h"
#include "android/hardware/radio/CellConnectionStatus.h"
#include "android/hardware/radio/CellIdentity.h"
#include "android/hardware/radio/CellIdentityCdma.h"
#include "android/hardware/radio/CellIdentityGsm.h"
#include "android/hardware/radio/CellIdentityLte.h"
#include "android/hardware/radio/CellIdentityNr.h"
#include "android/hardware/radio/CellIdentityOperatorNames.h"
#include "android/hardware/radio/CellIdentityTdscdma.h"
#include "android/hardware/radio/CellIdentityWcdma.h"
#include "android/hardware/radio/CellInfo.h"
#include "android/hardware/radio/CellInfoCdma.h"
#include "android/hardware/radio/CellInfoCellInfoRatSpecificInfo.h"
#include "android/hardware/radio/CellInfoGsm.h"
#include "android/hardware/radio/CellInfoInfo.h"
#include "android/hardware/radio/CellInfoLte.h"
#include "android/hardware/radio/CellInfoNr.h"
#include "android/hardware/radio/CellInfoTdscdma.h"
#include "android/hardware/radio/CellInfoType.h"
#include "android/hardware/radio/CellInfoWcdma.h"
#include "android/hardware/radio/CfData.h"
#include "android/hardware/radio/ClipStatus.h"
#include "android/hardware/radio/Clir.h"
#include "android/hardware/radio/ClosedSubscriberGroupInfo.h"
#include "android/hardware/radio/DataCallFailCause.h"
#include "android/hardware/radio/DataConnActiveStatus.h"
#include "android/hardware/radio/DataProfileId.h"
#include "android/hardware/radio/DataProfileInfo.h"
#include "android/hardware/radio/DataProfileInfoType.h"
#include "android/hardware/radio/DataRegStateResult.h"
#include "android/hardware/radio/DataRegStateResultVopsInfo.h"
#include "android/hardware/radio/DataRequestReason.h"
#include "android/hardware/radio/DataThrottlingAction.h"
#include "android/hardware/radio/DeviceStateType.h"
#include "android/hardware/radio/Dial.h"
#include "android/hardware/radio/Domain.h"
#include "android/hardware/radio/EmcIndicator.h"
#include "android/hardware/radio/EmergencyCallRouting.h"
#include "android/hardware/radio/EmergencyNumber.h"
#include "android/hardware/radio/EmergencyNumberSource.h"
#include "android/hardware/radio/EmergencyServiceCategory.h"
#include "android/hardware/radio/EmfIndicator.h"
#include "android/hardware/radio/EpsQos.h"
#include "android/hardware/radio/EutranBands.h"
#include "android/hardware/radio/EvdoSignalStrength.h"
#include "android/hardware/radio/FrequencyRange.h"
#include "android/hardware/radio/GeranBands.h"
#include "android/hardware/radio/GsmBroadcastSmsConfigInfo.h"
#include "android/hardware/radio/GsmSignalStrength.h"
#include "android/hardware/radio/GsmSmsMessage.h"
#include "android/hardware/radio/HandoverFailureMode.h"
#include "android/hardware/radio/HardwareConfig.h"
#include "android/hardware/radio/HardwareConfigModem.h"
#include "android/hardware/radio/HardwareConfigSim.h"
#include "android/hardware/radio/HardwareConfigState.h"
#include "android/hardware/radio/HardwareConfigType.h"
#include "android/hardware/radio/IccIo.h"
#include "android/hardware/radio/IccIoResult.h"
#include "android/hardware/radio/ImsSmsMessage.h"
#include "android/hardware/radio/ImsiEncryptionInfo.h"
#include "android/hardware/radio/IncrementalResultsPeriodicityRange.h"
#include "android/hardware/radio/IndicationFilter.h"
#include "android/hardware/radio/KeepaliveRequest.h"
#include "android/hardware/radio/KeepaliveStatus.h"
#include "android/hardware/radio/KeepaliveStatusCode.h"
#include "android/hardware/radio/KeepaliveType.h"
#include "android/hardware/radio/LastCallFailCause.h"
#include "android/hardware/radio/LastCallFailCauseInfo.h"
#include "android/hardware/radio/LceDataInfo.h"
#include "android/hardware/radio/LceStatus.h"
#include "android/hardware/radio/LceStatusInfo.h"
#include "android/hardware/radio/LinkAddress.h"
#include "android/hardware/radio/LinkCapacityEstimate.h"
#include "android/hardware/radio/LteSignalStrength.h"
#include "android/hardware/radio/LteVopsInfo.h"
#include "android/hardware/radio/MaxSearchTimeRange.h"
#include "android/hardware/radio/MaybePort.h"
#include "android/hardware/radio/MvnoType.h"
#include "android/hardware/radio/NeighboringCell.h"
#include "android/hardware/radio/NetworkScanRequest.h"
#include "android/hardware/radio/NetworkScanResult.h"
#include "android/hardware/radio/NgranBands.h"
#include "android/hardware/radio/NrDualConnectivityState.h"
#include "android/hardware/radio/NrIndicators.h"
#include "android/hardware/radio/NrQos.h"
#include "android/hardware/radio/NrSignalStrength.h"
#include "android/hardware/radio/NrVopsInfo.h"
#include "android/hardware/radio/NvItem.h"
#include "android/hardware/radio/NvWriteItem.h"
#include "android/hardware/radio/OperatorInfo.h"
#include "android/hardware/radio/OperatorStatus.h"
#include "android/hardware/radio/OptionalCsgInfo.h"
#include "android/hardware/radio/OptionalDnn.h"
#include "android/hardware/radio/OptionalOsAppId.h"
#include "android/hardware/radio/OptionalPdpProtocolType.h"
#include "android/hardware/radio/OptionalSliceInfo.h"
#include "android/hardware/radio/OptionalSscMode.h"
#include "android/hardware/radio/OptionalTrafficDescriptor.h"
#include "android/hardware/radio/OsAppId.h"
#include "android/hardware/radio/P2Constant.h"
#include "android/hardware/radio/PbReceivedStatus.h"
#include "android/hardware/radio/PcoDataInfo.h"
#include "android/hardware/radio/PdpProtocolType.h"
#include "android/hardware/radio/PersoSubstate.h"
#include "android/hardware/radio/PhoneRestrictedState.h"
#include "android/hardware/radio/PhonebookCapacity.h"
#include "android/hardware/radio/PhonebookRecordInfo.h"
#include "android/hardware/radio/PhysicalChannelConfig.h"
#include "android/hardware/radio/PhysicalChannelConfigBand.h"
#include "android/hardware/radio/PinState.h"
#include "android/hardware/radio/PortRange.h"
#include "android/hardware/radio/PreferredNetworkType.h"
#include "android/hardware/radio/PrlIndicator.h"
#include "android/hardware/radio/PublicKeyType.h"
#include "android/hardware/radio/Qos.h"
#include "android/hardware/radio/QosBandwidth.h"
#include "android/hardware/radio/QosFilter.h"
#include "android/hardware/radio/QosFilterDirection.h"
#include "android/hardware/radio/QosFilterIpsecSpi.h"
#include "android/hardware/radio/QosFilterIpv6FlowLabel.h"
#include "android/hardware/radio/QosFilterTypeOfService.h"
#include "android/hardware/radio/QosFlowIdRange.h"
#include "android/hardware/radio/QosPortRange.h"
#include "android/hardware/radio/QosProtocol.h"
#include "android/hardware/radio/QosSession.h"
#include "android/hardware/radio/RadioAccessFamily.h"
#include "android/hardware/radio/RadioAccessNetworks.h"
#include "android/hardware/radio/RadioAccessSpecifier.h"
#include "android/hardware/radio/RadioAccessSpecifierBands.h"
#include "android/hardware/radio/RadioBandMode.h"
#include "android/hardware/radio/RadioCapability.h"
#include "android/hardware/radio/RadioCapabilityPhase.h"
#include "android/hardware/radio/RadioCapabilityStatus.h"
#include "android/hardware/radio/RadioCdmaSmsConst.h"
#include "android/hardware/radio/RadioConst.h"
#include "android/hardware/radio/RadioError.h"
#include "android/hardware/radio/RadioFrequencyInfo.h"
#include "android/hardware/radio/RadioIndicationType.h"
#include "android/hardware/radio/RadioResponseInfo.h"
#include "android/hardware/radio/RadioResponseInfoModem.h"
#include "android/hardware/radio/RadioResponseType.h"
#include "android/hardware/radio/RadioState.h"
#include "android/hardware/radio/RadioTechnology.h"
#include "android/hardware/radio/RadioTechnologyFamily.h"
#include "android/hardware/radio/RegState.h"
#include "android/hardware/radio/RegStateResult.h"
#include "android/hardware/radio/RegStateResultAccessTechnologySpecificInfo.h"
#include "android/hardware/radio/RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo.h"
#include "android/hardware/radio/RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo.h"
#include "android/hardware/radio/RegistrationFailCause.h"
#include "android/hardware/radio/ResetNvType.h"
#include "android/hardware/radio/RestrictedState.h"
#include "android/hardware/radio/RouteSelectionDescriptor.h"
#include "android/hardware/radio/RouteSelectionDescriptorParams.h"
#include "android/hardware/radio/SapApduType.h"
#include "android/hardware/radio/SapConnectRsp.h"
#include "android/hardware/radio/SapDisconnectType.h"
#include "android/hardware/radio/SapResultCode.h"
#include "android/hardware/radio/SapStatus.h"
#include "android/hardware/radio/SapTransferProtocol.h"
#include "android/hardware/radio/ScanIntervalRange.h"
#include "android/hardware/radio/ScanStatus.h"
#include "android/hardware/radio/ScanType.h"
#include "android/hardware/radio/SelectUiccSub.h"
#include "android/hardware/radio/SendSmsResult.h"
#include "android/hardware/radio/SetupDataCallResult.h"
#include "android/hardware/radio/SignalMeasurementType.h"
#include "android/hardware/radio/SignalStrength.h"
#include "android/hardware/radio/SignalThresholdInfo.h"
#include "android/hardware/radio/SimApdu.h"
#include "android/hardware/radio/SimLockMultiSimPolicy.h"
#include "android/hardware/radio/SimRefreshResult.h"
#include "android/hardware/radio/SimRefreshType.h"
#include "android/hardware/radio/SliceInfo.h"
#include "android/hardware/radio/SliceServiceType.h"
#include "android/hardware/radio/SliceStatus.h"
#include "android/hardware/radio/SlicingConfig.h"
#include "android/hardware/radio/SmsAcknowledgeFailCause.h"
#include "android/hardware/radio/SmsWriteArgs.h"
#include "android/hardware/radio/SmsWriteArgsStatus.h"
#include "android/hardware/radio/SrvccState.h"
#include "android/hardware/radio/SsInfoData.h"
#include "android/hardware/radio/SsRequestType.h"
#include "android/hardware/radio/SsServiceType.h"
#include "android/hardware/radio/SsTeleserviceType.h"
#include "android/hardware/radio/SscMode.h"
#include "android/hardware/radio/StkCcUnsolSsResult.h"
#include "android/hardware/radio/SubscriptionType.h"
#include "android/hardware/radio/SuppServiceClass.h"
#include "android/hardware/radio/SuppSvcNotification.h"
#include "android/hardware/radio/TdscdmaSignalStrength.h"
#include "android/hardware/radio/TimeStampType.h"
#include "android/hardware/radio/TrafficDescriptor.h"
#include "android/hardware/radio/TtyMode.h"
#include "android/hardware/radio/UiccSubActStatus.h"
#include "android/hardware/radio/UrspRule.h"
#include "android/hardware/radio/UssdModeType.h"
#include "android/hardware/radio/UtranBands.h"
#include "android/hardware/radio/UusDcs.h"
#include "android/hardware/radio/UusInfo.h"
#include "android/hardware/radio/UusType.h"
#include "android/hardware/radio/VoiceRegStateResult.h"
#include "android/hardware/radio/VopsIndicator.h"
#include "android/hardware/radio/WcdmaSignalStrength.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIo& in, android::hardware::radio::IccIo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NeighboringCell& in,
        android::hardware::radio::NeighboringCell* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::UusInfo& in,
        android::hardware::radio::UusInfo* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_0::Dial& in,
                                                   android::hardware::radio::Dial* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LastCallFailCauseInfo& in,
        android::hardware::radio::LastCallFailCauseInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSignalStrength& in,
        android::hardware::radio::GsmSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalStrength& in,
        android::hardware::radio::CdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::EvdoSignalStrength& in,
        android::hardware::radio::EvdoSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SendSmsResult& in,
        android::hardware::radio::SendSmsResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::IccIoResult& in,
        android::hardware::radio::IccIoResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CallForwardInfo& in,
        android::hardware::radio::CallForwardInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::OperatorInfo& in,
        android::hardware::radio::OperatorInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SmsWriteArgs& in,
        android::hardware::radio::SmsWriteArgs* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAddress& in,
        android::hardware::radio::CdmaSmsAddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsSubaddress& in,
        android::hardware::radio::CdmaSmsSubaddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsMessage& in,
        android::hardware::radio::CdmaSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsAck& in,
        android::hardware::radio::CdmaSmsAck* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaBroadcastSmsConfigInfo& in,
        android::hardware::radio::CdmaBroadcastSmsConfigInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSmsWriteArgs& in,
        android::hardware::radio::CdmaSmsWriteArgs* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmBroadcastSmsConfigInfo& in,
        android::hardware::radio::GsmBroadcastSmsConfigInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::GsmSmsMessage& in,
        android::hardware::radio::GsmSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ImsSmsMessage& in,
        android::hardware::radio::ImsSmsMessage* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimApdu& in,
        android::hardware::radio::SimApdu* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::NvWriteItem& in,
        android::hardware::radio::NvWriteItem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SelectUiccSub& in,
        android::hardware::radio::SelectUiccSub* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigModem& in,
        android::hardware::radio::HardwareConfigModem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfigSim& in,
        android::hardware::radio::HardwareConfigSim* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::HardwareConfig& in,
        android::hardware::radio::HardwareConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceStatusInfo& in,
        android::hardware::radio::LceStatusInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::LceDataInfo& in,
        android::hardware::radio::LceDataInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::ActivityStatsInfo& in,
        android::hardware::radio::ActivityStatsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::Carrier& in,
        android::hardware::radio::Carrier* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CarrierRestrictions& in,
        android::hardware::radio::CarrierRestrictions* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SuppSvcNotification& in,
        android::hardware::radio::SuppSvcNotification* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SimRefreshResult& in,
        android::hardware::radio::SimRefreshResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaSignalInfoRecord& in,
        android::hardware::radio::CdmaSignalInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaCallWaiting& in,
        android::hardware::radio::CdmaCallWaiting* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaDisplayInfoRecord& in,
        android::hardware::radio::CdmaDisplayInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaNumberInfoRecord& in,
        android::hardware::radio::CdmaNumberInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaRedirectingNumberInfoRecord& in,
        android::hardware::radio::CdmaRedirectingNumberInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaLineControlInfoRecord& in,
        android::hardware::radio::CdmaLineControlInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53ClirInfoRecord& in,
        android::hardware::radio::CdmaT53ClirInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaT53AudioControlInfoRecord& in,
        android::hardware::radio::CdmaT53AudioControlInfoRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecord& in,
        android::hardware::radio::CdmaInformationRecord* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CdmaInformationRecords& in,
        android::hardware::radio::CdmaInformationRecords* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::CfData& in, android::hardware::radio::CfData* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::SsInfoData& in,
        android::hardware::radio::SsInfoData* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::StkCcUnsolSsResult& in,
        android::hardware::radio::StkCcUnsolSsResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_0::PcoDataInfo& in,
        android::hardware::radio::PcoDataInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveRequest& in,
        android::hardware::radio::KeepaliveRequest* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_1::KeepaliveStatus& in,
        android::hardware::radio::KeepaliveStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityOperatorNames& in,
        android::hardware::radio::CellIdentityOperatorNames* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellIdentityCdma& in,
        android::hardware::radio::CellIdentityCdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::CellInfoCdma& in,
        android::hardware::radio::CellInfoCdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::WcdmaSignalStrength& in,
        android::hardware::radio::WcdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::TdscdmaSignalStrength& in,
        android::hardware::radio::TdscdmaSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& in,
        android::hardware::radio::VoiceRegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_3::RadioResponseInfoModem& in,
        android::hardware::radio::RadioResponseInfoModem* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::EmergencyNumber& in,
        android::hardware::radio::EmergencyNumber* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioFrequencyInfo& in,
        android::hardware::radio::RadioFrequencyInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::LteVopsInfo& in,
        android::hardware::radio::LteVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::NrIndicators& in,
        android::hardware::radio::NrIndicators* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult& in,
        android::hardware::radio::DataRegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo& in,
        android::hardware::radio::DataRegStateResultVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellConfigLte& in,
        android::hardware::radio::CellConfigLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CellInfo::Info& in,
        android::hardware::radio::CellInfoInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::RadioCapability& in,
        android::hardware::radio::RadioCapability* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_4::CarrierRestrictionsWithPriority& in,
        android::hardware::radio::CarrierRestrictionsWithPriority* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier& in,
        android::hardware::radio::RadioAccessSpecifier* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands& in,
        android::hardware::radio::RadioAccessSpecifierBands* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::SignalThresholdInfo& in,
        android::hardware::radio::SignalThresholdInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::NetworkScanRequest& in,
        android::hardware::radio::NetworkScanRequest* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::DataProfileInfo& in,
        android::hardware::radio::DataProfileInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::LinkAddress& in,
        android::hardware::radio::LinkAddress* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::ClosedSubscriberGroupInfo& in,
        android::hardware::radio::ClosedSubscriberGroupInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::OptionalCsgInfo& in,
        android::hardware::radio::OptionalCsgInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityGsm& in,
        android::hardware::radio::CellIdentityGsm* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityWcdma& in,
        android::hardware::radio::CellIdentityWcdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityTdscdma& in,
        android::hardware::radio::CellIdentityTdscdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityLte& in,
        android::hardware::radio::CellIdentityLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentityNr& in,
        android::hardware::radio::CellIdentityNr* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoGsm& in,
        android::hardware::radio::CellInfoGsm* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoWcdma& in,
        android::hardware::radio::CellInfoWcdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellInfoTdscdma& in,
        android::hardware::radio::CellInfoTdscdma* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CellIdentity& in,
        android::hardware::radio::CellIdentity* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo& in,
        android::hardware::radio::BarringInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo::Conditional&
                in,
        android::hardware::radio::BarringInfoBarringTypeSpecificInfoConditional* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::BarringInfo::BarringTypeSpecificInfo& in,
        android::hardware::radio::BarringInfoBarringTypeSpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                Cdma2000RegistrationInfo& in,
        android::hardware::radio::
                RegStateResultAccessTechnologySpecificInfoCdma2000RegistrationInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::RegStateResult::AccessTechnologySpecificInfo::
                EutranRegistrationInfo& in,
        android::hardware::radio::RegStateResultAccessTechnologySpecificInfoEutranRegistrationInfo*
                out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::AppStatus& in,
        android::hardware::radio::AppStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_5::CardStatus& in,
        android::hardware::radio::CardStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosBandwidth& in,
        android::hardware::radio::QosBandwidth* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::EpsQos& in, android::hardware::radio::EpsQos* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrQos& in, android::hardware::radio::NrQos* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Qos& in,
                                                   android::hardware::radio::Qos* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& in,
        android::hardware::radio::RadioResponseInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PortRange& in,
        android::hardware::radio::PortRange* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::MaybePort& in,
        android::hardware::radio::MaybePort* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter& in,
        android::hardware::radio::QosFilter* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::TypeOfService& in,
        android::hardware::radio::QosFilterTypeOfService* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::Ipv6FlowLabel& in,
        android::hardware::radio::QosFilterIpv6FlowLabel* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosFilter::IpsecSpi& in,
        android::hardware::radio::QosFilterIpsecSpi* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::QosSession& in,
        android::hardware::radio::QosSession* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SetupDataCallResult& in,
        android::hardware::radio::SetupDataCallResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LinkCapacityEstimate& in,
        android::hardware::radio::LinkCapacityEstimate* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrVopsInfo& in,
        android::hardware::radio::NrVopsInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::LteSignalStrength& in,
        android::hardware::radio::LteSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NrSignalStrength& in,
        android::hardware::radio::NrSignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SignalStrength& in,
        android::hardware::radio::SignalStrength* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoLte& in,
        android::hardware::radio::CellInfoLte* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfoNr& in,
        android::hardware::radio::CellInfoNr* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo& in,
        android::hardware::radio::CellInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::CellInfo::CellInfoRatSpecificInfo& in,
        android::hardware::radio::CellInfoCellInfoRatSpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::NetworkScanResult& in,
        android::hardware::radio::NetworkScanResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult& in,
        android::hardware::radio::RegStateResult* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RegStateResult::AccessTechnologySpecificInfo& in,
        android::hardware::radio::RegStateResultAccessTechnologySpecificInfo* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::radio::V1_6::Call& in,
                                                   android::hardware::radio::Call* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig& in,
        android::hardware::radio::PhysicalChannelConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhysicalChannelConfig::Band& in,
        android::hardware::radio::PhysicalChannelConfigBand* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSliceInfo& in,
        android::hardware::radio::OptionalSliceInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SliceInfo& in,
        android::hardware::radio::SliceInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalDnn& in,
        android::hardware::radio::OptionalDnn* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalOsAppId& in,
        android::hardware::radio::OptionalOsAppId* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalTrafficDescriptor& in,
        android::hardware::radio::OptionalTrafficDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::TrafficDescriptor& in,
        android::hardware::radio::TrafficDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OsAppId& in,
        android::hardware::radio::OsAppId* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::SlicingConfig& in,
        android::hardware::radio::SlicingConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::UrspRule& in,
        android::hardware::radio::UrspRule* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RouteSelectionDescriptor& in,
        android::hardware::radio::RouteSelectionDescriptor* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::RouteSelectionDescriptorParams& in,
        android::hardware::radio::RouteSelectionDescriptorParams* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalPdpProtocolType& in,
        android::hardware::radio::OptionalPdpProtocolType* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::OptionalSscMode& in,
        android::hardware::radio::OptionalSscMode* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::ImsiEncryptionInfo& in,
        android::hardware::radio::ImsiEncryptionInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookRecordInfo& in,
        android::hardware::radio::PhonebookRecordInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::V1_6::PhonebookCapacity& in,
        android::hardware::radio::PhonebookCapacity* out);

}  // namespace android::h2a
