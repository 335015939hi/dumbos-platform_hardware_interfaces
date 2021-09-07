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

#pragma once

#include <limits>
#include "android/hardware/radio/config/1.0/types.h"
#include "android/hardware/radio/config/1.1/types.h"
#include "android/hardware/radio/config/1.2/types.h"
#include "android/hardware/radio/config/1.3/types.h"
#include "android/hardware/radio/config/HalDeviceCapabilities.h"
#include "android/hardware/radio/config/ModemInfo.h"
#include "android/hardware/radio/config/ModemsConfig.h"
#include "android/hardware/radio/config/PhoneCapability.h"
#include "android/hardware/radio/config/SimSlotStatus.h"
#include "android/hardware/radio/config/SlotState.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_2::SimSlotStatus& in,
        android::hardware::radio::config::SimSlotStatus* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::PhoneCapability& in,
        android::hardware::radio::config::PhoneCapability* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::ModemInfo& in,
        android::hardware::radio::config::ModemInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_1::ModemsConfig& in,
        android::hardware::radio::config::ModemsConfig* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::radio::config::V1_3::HalDeviceCapabilities& in,
        android::hardware::radio::config::HalDeviceCapabilities* out);

}  // namespace android::h2a
