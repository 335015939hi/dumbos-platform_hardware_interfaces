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

#include "android/hardware/radio/config/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::radio::config::SlotState::INACTIVE ==
              static_cast<aidl::android::hardware::radio::config::SlotState>(
                      ::android::hardware::radio::config::V1_0::SlotState::INACTIVE));
static_assert(aidl::android::hardware::radio::config::SlotState::ACTIVE ==
              static_cast<aidl::android::hardware::radio::config::SlotState>(
                      ::android::hardware::radio::config::V1_0::SlotState::ACTIVE));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_2::SimSlotStatus& in,
        aidl::android::hardware::radio::config::SimSlotStatus* out) {
    out->cardState = static_cast<aidl::android::hardware::radio::CardState>(in.base.cardState);
    out->slotState =
            static_cast<aidl::android::hardware::radio::config::SlotState>(in.base.slotState);
    out->atr = in.base.atr;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.base.logicalSlotId > std::numeric_limits<int32_t>::max() || in.base.logicalSlotId < 0) {
        return false;
    }
    out->logicalSlotId = static_cast<int32_t>(in.base.logicalSlotId);
    out->iccid = in.base.iccid;
    out->eid = in.eid;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::PhoneCapability& in,
        aidl::android::hardware::radio::config::PhoneCapability* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.maxActiveData > std::numeric_limits<int8_t>::max() || in.maxActiveData < 0) {
        return false;
    }
    out->maxActiveData = static_cast<int8_t>(in.maxActiveData);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.maxActiveInternetData > std::numeric_limits<int8_t>::max() ||
        in.maxActiveInternetData < 0) {
        return false;
    }
    out->maxActiveInternetData = static_cast<int8_t>(in.maxActiveInternetData);
    out->isInternetLingeringSupported = static_cast<bool>(in.isInternetLingeringSupported);
    size_t size = in.logicalModemList.size();
    aidl::android::hardware::radio::UusInfo uusInfo;
    for (size_t i = 0; i < size; i++) {
        if (!translate(in.logicalModemList[i], &uusInfo)) return false;
        out->uusInfo.push_back(uusInfo);
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::ModemInfo& in,
        aidl::android::hardware::radio::config::ModemInfo* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.modemId > std::numeric_limits<int8_t>::max() || in.modemId < 0) {
        return false;
    }
    out->modemId = static_cast<int8_t>(in.modemId);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::ModemsConfig& in,
        aidl::android::hardware::radio::config::ModemsConfig* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit
    // your needs.
    if (in.numOfLiveModems > std::numeric_limits<int8_t>::max() || in.numOfLiveModems < 0) {
        return false;
    }
    out->numOfLiveModems = static_cast<int8_t>(in.numOfLiveModems);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_3::HalDeviceCapabilities& in,
        aidl::android::hardware::radio::config::HalDeviceCapabilities* out) {
    out->modemReducedFeatureSet1 = static_cast<bool>(in.modemReducedFeatureSet1);
    return true;
}

}  // namespace android::h2a