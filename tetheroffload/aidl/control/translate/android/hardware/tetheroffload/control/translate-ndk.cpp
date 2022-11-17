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

#include "android/hardware/tetheroffload/control/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::tetheroffload::control::NetworkProtocol::TCP == static_cast<aidl::android::hardware::tetheroffload::control::NetworkProtocol>(::android::hardware::tetheroffload::control::V1_0::NetworkProtocol::TCP));
static_assert(aidl::android::hardware::tetheroffload::control::NetworkProtocol::UDP == static_cast<aidl::android::hardware::tetheroffload::control::NetworkProtocol>(::android::hardware::tetheroffload::control::V1_0::NetworkProtocol::UDP));

static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_STARTED == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_STARTED));
static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_STOPPED_ERROR == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_STOPPED_ERROR));
static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_STOPPED_UNSUPPORTED == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_STOPPED_UNSUPPORTED));
static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_SUPPORT_AVAILABLE == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_SUPPORT_AVAILABLE));
static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_STOPPED_LIMIT_REACHED == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_STOPPED_LIMIT_REACHED));
static_assert(aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent::OFFLOAD_WARNING_REACHED == static_cast<aidl::android::hardware::tetheroffload::control::OffloadCallbackEvent>(::android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent::OFFLOAD_WARNING_REACHED));

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair& in, aidl::android::hardware::tetheroffload::control::IPv4AddrPortPair* out) {
    out->addr = in.addr;
    out->port = static_cast<char16_t>(in.port);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate& in, aidl::android::hardware::tetheroffload::control::NatTimeoutUpdate* out) {
    if (!translate(in.src, &out->src)) return false;
    if (!translate(in.dst, &out->dst)) return false;
    out->proto = static_cast<aidl::android::hardware::tetheroffload::control::NetworkProtocol>(in.proto);
    return true;
}

}  // namespace android::h2a