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

#include <libminradio/RadioSim.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>
#include <libminradio/sim/IccConstants.h>
#include <libminradio/sim/structs.h>

#define RADIO_MODULE "Sim"

namespace android::hardware::radio::minimal {

using ::ndk::ScopedAStatus;
using namespace ::android::hardware::radio::minimal::structs;
namespace aidl = ::aidl::android::hardware::radio::sim;
constexpr auto ok = &ScopedAStatus::ok;

RadioSim::RadioSim() {}

void RadioSim::setIccid(std::string iccid) {
    using namespace sim::icc::constants;

    mIccid = iccid;
    mIcc[{
            .command = COMMAND_READ_BINARY,
            .fileId = EF_ICCID,
            .path = MF_SIM,
            .p3 = GET_RESPONSE_EF_IMG_SIZE_BYTES,
    }] = iccid;
}

ScopedAStatus RadioSim::areUiccApplicationsEnabled(int32_t serial) {
    LOG_CALL;
    respond->areUiccApplicationsEnabledResponse(noError(serial), mAreUiccApplicationsEnabled);
    return ok();
}

ScopedAStatus RadioSim::changeIccPin2ForApp(int32_t serial, const std::string& oldPin2,
                                            const std::string& newPin2, const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::changeIccPinForApp(int32_t serial, const std::string& oldPin,
                                           const std::string& newPin, const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::enableUiccApplications(int32_t serial, bool enable) {
    LOG_CALL << enable;
    mAreUiccApplicationsEnabled = enable;
    respond->enableUiccApplicationsResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioSim::getAllowedCarriers(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::getCdmaSubscription(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::getCdmaSubscriptionSource(int32_t serial) {
    LOG_CALL;
    // TODO: use info from setCdmaSubscriptionSource?
    respond->getCdmaSubscriptionSourceResponse(noError(serial),
                                               aidl::CdmaSubscriptionSource::RUIM_SIM);
    return ok();
}

ScopedAStatus RadioSim::getFacilityLockForApp(  //
        int32_t serial, const std::string& facility, const std::string& password,
        int32_t serviceClass, const std::string& appId) {
    LOG_CALL << facility << ' ' << serviceClass << ' ' << appId;
    respond->getFacilityLockForAppResponse(noError(serial), 0);  // 0 means "disabled for all"
    return ok();
}

ScopedAStatus RadioSim::getSimPhonebookCapacity(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::getSimPhonebookRecords(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
// TODO: not implement and make sure framework doesn't call this
ScopedAStatus RadioSim::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {
    LOG_CALL << channelId;
    respond->iccCloseLogicalChannelResponse(noError(serial));
    return ok();
}
#pragma clang diagnostic pop

ScopedAStatus RadioSim::iccCloseLogicalChannelWithSessionInfo(
        int32_t serial, const aidl::SessionInfo& sessionInfo) {
    LOG_CALL << sessionInfo;
    respond->iccCloseLogicalChannelWithSessionInfoResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioSim::iccIoForApp(int32_t serial, const aidl::IccIo& iccIo) {
    LOG_CALL << iccIo;

    using namespace sim::icc::constants;

    aidl::IccIoResult iccResult;

    auto it = mIcc.find(iccIo);
    if (it != mIcc.end()) {
        iccResult = toIccIoResult(it->second);
    } else {
        LOG(DEBUG) << "Missing ICC file: " << iccIo;
        iccResult = toIccIoResult(IO_RESULT_FILE_NOT_FOUND);
    }

    respond->iccIoForAppResponse(noError(serial), iccResult);
    return ok();
}

ScopedAStatus RadioSim::iccOpenLogicalChannel(int32_t serial, const std::string& aid, int32_t p2) {
    LOG_CALL << aid << ' ' << p2;
    respond->iccOpenLogicalChannelResponse(notSupported(serial), 0, {});
    return ok();
}

ScopedAStatus RadioSim::iccTransmitApduBasicChannel(int32_t serial, const aidl::SimApdu& message) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::iccTransmitApduLogicalChannel(int32_t serial,
                                                      const aidl::SimApdu& message) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::reportStkServiceIsRunning(int32_t serial) {
    LOG_CALL;
    respond->reportStkServiceIsRunningResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioSim::requestIccSimAuthentication(  //
        int32_t serial, int32_t authContext, const std::string& authData, const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::responseAcknowledgement() {
    LOG_CALL_NOSERIAL;
    return ok();
}

ScopedAStatus RadioSim::sendEnvelope(int32_t serial, const std::string& command) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::sendEnvelopeWithStatus(int32_t serial, const std::string& contents) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::sendTerminalResponseToSim(int32_t serial,
                                                  const std::string& commandResponse) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::setAllowedCarriers(  //
        int32_t serial, const aidl::CarrierRestrictions& carriers, aidl::SimLockMultiSimPolicy mp) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::setCarrierInfoForImsiEncryption(
        int32_t serial, const aidl::ImsiEncryptionInfo& imsiEncryptionInfo) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::setCdmaSubscriptionSource(int32_t serial,
                                                  aidl::CdmaSubscriptionSource cdmaSub) {
    LOG_CALL << static_cast<int>(cdmaSub);
    const bool isSim = (cdmaSub == aidl::CdmaSubscriptionSource::RUIM_SIM);
    respond->setCdmaSubscriptionSourceResponse(isSim ? noError(serial) : notSupported(serial));
    return ok();
}

ScopedAStatus RadioSim::setFacilityLockForApp(  //
        int32_t serial, const std::string& facility, bool lockState, const std::string& password,
        int32_t serviceClass, const std::string& appId) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioSimResponse>& response,
        const std::shared_ptr<aidl::IRadioSimIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;
    return ok();
}

ScopedAStatus RadioSim::setSimCardPower(int32_t serial, aidl::CardPowerState powerUp) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::setUiccSubscription(int32_t serial, const aidl::SelectUiccSub& uiccSub) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::supplyIccPin2ForApp(int32_t serial, const std::string& pin2,
                                            const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::supplyIccPinForApp(int32_t serial, const std::string& pin,
                                           const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::supplyIccPuk2ForApp(int32_t serial, const std::string& puk2,
                                            const std::string& pin2, const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::supplyIccPukForApp(int32_t serial, const std::string& puk,
                                           const std::string& pin, const std::string& aid) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::supplySimDepersonalization(int32_t serial, aidl::PersoSubstate pss,
                                                   const std::string& controlKey) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioSim::updateSimPhonebookRecords(int32_t serial,
                                                  const aidl::PhonebookRecordInfo& recordInfo) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

}  // namespace android::hardware::radio::minimal
