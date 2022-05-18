/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include "aidl/android/hardware/tv/cec/AbortReason.h"
#include "aidl/android/hardware/tv/cec/CecDeviceType.h"
#include "aidl/android/hardware/tv/cec/CecLogicalAddress.h"
#include "aidl/android/hardware/tv/cec/CecMessage.h"
#include "aidl/android/hardware/tv/cec/CecMessageType.h"
#include "aidl/android/hardware/tv/cec/HdmiPortInfo.h"
#include "aidl/android/hardware/tv/cec/HdmiPortType.h"
#include "aidl/android/hardware/tv/cec/HotplugEvent.h"
#include "aidl/android/hardware/tv/cec/MaxLength.h"
#include "aidl/android/hardware/tv/cec/OptionKey.h"
#include "aidl/android/hardware/tv/cec/Result.h"
#include "aidl/android/hardware/tv/cec/SendMessageResult.h"
#include "android/hardware/tv/cec/1.0/types.h"
#include "android/hardware/tv/cec/1.1/types.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_0::HotplugEvent& in,
        aidl::android::hardware::tv::cec::HotplugEvent* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_0::HdmiPortInfo& in,
        aidl::android::hardware::tv::cec::HdmiPortInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tv::cec::V1_1::CecMessage& in,
        aidl::android::hardware::tv::cec::CecMessage* out);

}  // namespace android::h2a
