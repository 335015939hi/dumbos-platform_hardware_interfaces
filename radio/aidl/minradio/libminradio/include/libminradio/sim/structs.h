/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/hardware/radio/sim/IccIo.h>
#include <aidl/android/hardware/radio/sim/IccIoResult.h>
#include <aidl/android/hardware/radio/sim/SessionInfo.h>

#include <string>

namespace android::hardware::radio::minimal::structs {

::aidl::android::hardware::radio::sim::IccIoResult toIccIoResult(std::string_view simResponse);
::aidl::android::hardware::radio::sim::IccIoResult toIccIoResult(std::pair<int, int> pair);

std::ostream& operator<<(std::ostream& os,
                         const ::aidl::android::hardware::radio::sim::IccIo& iccIo);

std::ostream& operator<<(std::ostream& os,
                         const ::aidl::android::hardware::radio::sim::SessionInfo& sessionInfo);

}  // namespace android::hardware::radio::minimal::structs
