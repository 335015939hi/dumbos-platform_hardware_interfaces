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

#include "libradiocompat/RadioSimSap.h"

#include "commonStructs.h"
#include "structs.h"

#include "collections.h"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::sim;
constexpr auto ok = &ScopedAStatus::ok;

RadioSimSap::RadioSimSap(sp<V1_0::ISap> hidlHal)
    : mSap(hidlHal), mSapCallback(sp<RadioSimSapCallback>::make()) {}

ScopedAStatus RadioSimSap::apduReq(int32_t token, aidl::SapApduType type,
                                   const std::vector<uint8_t>& command) {
    mSap->apduReq(token, toHidl(type), toHidl(command));
    return ok();
}

ScopedAStatus RadioSimSap::connectReq(int32_t token, int32_t maxMsgSize) {
    mSap->connectReq(token, maxMsgSize);
    return ok();
}

ScopedAStatus RadioSimSap::disconnectReq(int32_t token) {
    mSap->disconnectReq(token);
    return ok();
}

ScopedAStatus RadioSimSap::powerReq(int32_t token, bool state) {
    mSap->powerReq(token, state);
    return ok();
}

ScopedAStatus RadioSimSap::resetSimReq(int32_t token) {
    mSap->resetSimReq(token);
    return ok();
}

ScopedAStatus RadioSimSap::setCallback(
        const std::shared_ptr<::aidl::android::hardware::radio::sim::ISapCallback>& sapCallback) {
    mSapCallback->setResponseFunction(sapCallback);
    mSap->setCallback(mSapCallback);
    return ok();
}
ScopedAStatus RadioSimSap::setTransferProtocolReq(int32_t token,
                                                  aidl::SapTransferProtocol transferProtocol) {
    mSap->setTransferProtocolReq(token, toHidl(transferProtocol));
    return ok();
}

ScopedAStatus RadioSimSap::transferAtrReq(int32_t token) {
    mSap->transferAtrReq(token);
    return ok();
}
ScopedAStatus RadioSimSap::transferCardReaderStatusReq(int32_t token) {
    mSap->transferCardReaderStatusReq(token);
    return ok();
}

}  // namespace android::hardware::radio::compat
