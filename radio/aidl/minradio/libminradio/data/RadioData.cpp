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

#include <libminradio/RadioData.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "Data"

namespace android::hardware::radio::minimal {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::data;
constexpr auto ok = &ScopedAStatus::ok;

RadioData::RadioData() {}

ScopedAStatus RadioData::allocatePduSessionId(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::cancelHandover(int32_t serial, int32_t callId) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::deactivateDataCall(int32_t serial, int32_t cid,
                                            aidl::DataRequestReason reason) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::getDataCallList(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::getSlicingConfig(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::releasePduSessionId(int32_t serial, int32_t id) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::responseAcknowledgement() {
    LOG_CALL_NOSERIAL;
    return ok();
}

ScopedAStatus RadioData::setDataAllowed(int32_t serial, bool allow) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::setDataProfile(int32_t serial,
                                        const std::vector<aidl::DataProfileInfo>& profiles) {
    LOG_CALL;
    respond->setDataProfileResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioData::setDataThrottling(int32_t serial, aidl::DataThrottlingAction dta,
                                           int64_t completionDurationMs) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::setInitialAttachApn(int32_t serial,
                                             const std::optional<aidl::DataProfileInfo>& info) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioDataResponse>& response,
        const std::shared_ptr<aidl::IRadioDataIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;
    return ok();
}

ScopedAStatus RadioData::startHandover(int32_t serial, int32_t callId) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::startKeepalive(int32_t serial, const aidl::KeepaliveRequest& keepalive) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioData::stopKeepalive(int32_t serial, int32_t sessionHandle) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

}  // namespace android::hardware::radio::minimal
