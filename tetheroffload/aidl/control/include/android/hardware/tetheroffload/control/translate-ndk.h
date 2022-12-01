/**
 *
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include "aidl/android/hardware/tetheroffload/control/IPv4AddrPortPair.h"
#include "aidl/android/hardware/tetheroffload/control/NatTimeoutUpdate.h"
#include "aidl/android/hardware/tetheroffload/control/NetworkProtocol.h"
#include "aidl/android/hardware/tetheroffload/control/OffloadCallbackEvent.h"
#include "android/hardware/tetheroffload/control/1.0/types.h"
#include "android/hardware/tetheroffload/control/1.1/types.h"
#include <limits>

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair& in, aidl::android::hardware::tetheroffload::control::IPv4AddrPortPair* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate& in, aidl::android::hardware::tetheroffload::control::NatTimeoutUpdate* out);

}  // namespace android::h2a