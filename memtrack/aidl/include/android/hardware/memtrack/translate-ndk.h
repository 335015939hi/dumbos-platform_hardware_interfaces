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

#include <aidl/android/hardware/memtrack/MemtrackRecord.h>
#include <aidl/android/hardware/memtrack/MemtrackType.h>
#include <android/hardware/memtrack/1.0/types.h>

namespace V1_0_hidl = ::android::hardware::memtrack::V1_0;
namespace V_aidl = ::aidl::android::hardware::memtrack;

namespace aidl::android::hardware::memtrack::h2a {

__attribute__((warn_unused_result)) bool translate(const V1_0_hidl::MemtrackRecord& in,
                                                   V_aidl::MemtrackRecord* out);

}  // namespace aidl::android::hardware::memtrack::h2a
