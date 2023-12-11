/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <libminradio/RadioNetwork.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "Network"

namespace android::hardware::radio::minimal {

using ::aidl::android::hardware::radio::AccessNetwork;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::network;
namespace aidlRadio = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

// TODO: move to some common library
constexpr int kRadioIntMax = 0x7FFFFFFF;

RadioNetwork::RadioNetwork() {}

ScopedAStatus RadioNetwork::getAllowedNetworkTypesBitmap(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getAvailableBandModes(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getAvailableNetworks(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getBarringInfo(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getCdmaRoamingPreference(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getCellInfoList(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getImsRegistrationState(int32_t serial) {
    LOG_CALL;
#pragma clang diagnostic push
    // TODO: why is the framework calling deprecated function?
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    respond->getImsRegistrationStateResponse(noError(serial),
                                             /*isRegistered*/ true,  // TODO: try saying false
                                             aidlRadio::RadioTechnologyFamily::THREE_GPP);
#pragma clang diagnostic pop
    return ok();
}

ScopedAStatus RadioNetwork::getNetworkSelectionMode(int32_t serial) {
    LOG_CALL;
    respond->getNetworkSelectionModeResponse(noError(serial), false);  // automatic
    return ok();
}

ScopedAStatus RadioNetwork::getSystemSelectionChannels(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getVoiceRadioTechnology(int32_t serial) {
    LOG_CALL;
    respond->getVoiceRadioTechnologyResponse(noError(serial), aidlRadio::RadioTechnology::LTE);
    return ok();
}

// TODO: return no voice signal
ScopedAStatus RadioNetwork::getVoiceRegistrationState(int32_t serial) {
    LOG_CALL;
    // TODO: remove?
    aidl::CellIdentityLte cellid{
            .mcc = "300",
            .mnc = "555",
            .ci = 12345,
            .pci = 102,
            .tac = kRadioIntMax,
            .earfcn = 103,
            .operatorNames =
                    {
                            .alphaLong = "Minradio",
                            .alphaShort = "MR",
                            .operatorNumeric = "300555",
                            .status = aidl::OperatorInfo::STATUS_CURRENT,
                    },
            .bandwidth = 104,
            .additionalPlmns = {},
            .csgInfo = std::nullopt,
            .bands =
                    {
                            aidl::EutranBands::BAND_1,
                            aidl::EutranBands::BAND_88,
                    },
    };
    aidl::RegStateResult res{
            .regState = aidl::RegState::REG_HOME,
            .rat = aidlRadio::RadioTechnology::LTE,
            .reasonForDenial = aidl::RegistrationFailCause::NONE,
            .cellIdentity = cellid,
            .registeredPlmn = "something",
    };
    respond->getVoiceRegistrationStateResponse(noError(serial), res);
    return ok();
}

ScopedAStatus RadioNetwork::isNrDualConnectivityEnabled(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::responseAcknowledgement() {
    LOG_CALL_NOSERIAL;
    return ok();
}

ScopedAStatus RadioNetwork::setAllowedNetworkTypesBitmap(int32_t serial, int32_t ntype) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setBandMode(int32_t serial, aidl::RadioBandMode mode) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setBarringPassword(int32_t serial, const std::string& facility,
                                               const std::string& oldPw, const std::string& newPw) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setCdmaRoamingPreference(int32_t serial, aidl::CdmaRoamingType type) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setCellInfoListRate(int32_t serial, int32_t rate) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setIndicationFilter(int32_t serial, int32_t indFilter) {
    LOG_CALL;
    respond->setIndicationFilterResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioNetwork::setLinkCapacityReportingCriteria(  //
        int32_t serial, int32_t hysteresisMs, int32_t hysteresisDlKbps, int32_t hysteresisUlKbps,
        const std::vector<int32_t>& thrDownlinkKbps, const std::vector<int32_t>& thrUplinkKbps,
        AccessNetwork accessNetwork) {
    LOG_CALL;
    respond->setLinkCapacityReportingCriteriaResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioNetwork::setLocationUpdates(int32_t serial, bool enable) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeAutomatic(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeManual(  //
        int32_t serial, const std::string& opNumeric, AccessNetwork ran) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setNrDualConnectivityState(int32_t serial,
                                                       aidl::NrDualConnectivityState st) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioNetworkResponse>& response,
        const std::shared_ptr<aidl::IRadioNetworkIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;
    return ok();
}

ScopedAStatus RadioNetwork::setSignalStrengthReportingCriteria(
        int32_t serial, const std::vector<aidl::SignalThresholdInfo>& infos) {
    LOG_CALL;
    respond->setSignalStrengthReportingCriteriaResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioNetwork::setSuppServiceNotifications(int32_t serial, bool enable) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setSystemSelectionChannels(  //
        int32_t serial, bool specifyCh, const std::vector<aidl::RadioAccessSpecifier>& specifiers) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::startNetworkScan(int32_t serial, const aidl::NetworkScanRequest& req) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::stopNetworkScan(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::supplyNetworkDepersonalization(int32_t serial,
                                                           const std::string& nPin) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setUsageSetting(int32_t serial, aidl::UsageSetting) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::getUsageSetting(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setEmergencyMode(int32_t serial, aidl::EmergencyMode) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::triggerEmergencyNetworkScan(int32_t serial,
                                                        const aidl::EmergencyNetworkScanTrigger&) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::cancelEmergencyNetworkScan(int32_t serial, bool) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::exitEmergencyMode(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::setNullCipherAndIntegrityEnabled(int32_t serial, bool) {
    LOG_CALL;
    respond->setNullCipherAndIntegrityEnabledResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioNetwork::isNullCipherAndIntegrityEnabled(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioNetwork::isN1ModeEnabled(int32_t serial) {
    LOG_CALL;
    respond->isN1ModeEnabledResponse(noError(serial), true);  // TODO: false
    return ok();
}

ScopedAStatus RadioNetwork::setN1ModeEnabled(int32_t serial, bool /*enable*/) {
    LOG_CALL;
    respond->setN1ModeEnabledResponse(noError(serial));  // TODO: notsupported
    return ok();
}

}  // namespace android::hardware::radio::minimal
