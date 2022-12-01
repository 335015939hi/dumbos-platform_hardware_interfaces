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

#include <libradiocompat/RadioSimSapCallback.h>

#include "commonStructs.h"
#include "structs.h"

#include "collections.h"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sim;

void RadioSimSapCallback::setResponseFunction(std::shared_ptr<aidl::ISapCallback> callback) {
    mCallback = callback;
}

std::shared_ptr<aidl::ISapCallback> RadioSimSapCallback::respond() {
    return mCallback.get();
}

Return<void> RadioSimSapCallback::apduResponse(int32_t token, V1_0::SapResultCode resultCode,
                                               const hidl_vec<uint8_t>& apduRsp) {
    respond()->apduResponse(token, toAidl(resultCode), toAidl(apduRsp));
    return {};
}

Return<void> RadioSimSapCallback::connectResponse(int32_t token, V1_0::SapConnectRsp sapConnectRsp,
                                                  int32_t maxMsgSize) {
    respond()->connectResponse(token, toAidl(sapConnectRsp), maxMsgSize);
    return {};
}

Return<void> RadioSimSapCallback::disconnectIndication(int32_t token,
                                                       V1_0::SapDisconnectType disconnectType) {
    respond()->disconnectIndication(token, toAidl(disconnectType));
    return {};
}

Return<void> RadioSimSapCallback::disconnectResponse(int32_t token) {
    respond()->disconnectResponse(token);
    return {};
}

Return<void> RadioSimSapCallback::errorResponse(int32_t token) {
    respond()->errorResponse(token);
    return {};
}

Return<void> RadioSimSapCallback::powerResponse(int32_t token, V1_0::SapResultCode resultCode) {
    respond()->powerResponse(token, toAidl(resultCode));
    return {};
}

Return<void> RadioSimSapCallback::resetSimResponse(int32_t token, V1_0::SapResultCode resultCode) {
    respond()->resetSimResponse(token, toAidl(resultCode));
    return {};
}

Return<void> RadioSimSapCallback::statusIndication(int32_t token, V1_0::SapStatus status) {
    respond()->statusIndication(token, toAidl(status));
    return {};
}

Return<void> RadioSimSapCallback::transferAtrResponse(int32_t token, V1_0::SapResultCode resultCode,
                                                      const hidl_vec<uint8_t>& atr) {
    respond()->transferAtrResponse(token, toAidl(resultCode), toAidl(atr));
    return {};
}

Return<void> RadioSimSapCallback::transferCardReaderStatusResponse(int32_t token,
                                                                   V1_0::SapResultCode resultCode,
                                                                   int32_t cardReaderStatus) {
    respond()->transferCardReaderStatusResponse(token, toAidl(resultCode), cardReaderStatus);
    return {};
}

Return<void> RadioSimSapCallback::transferProtocolResponse(int32_t token,
                                                           V1_0::SapResultCode resultCode) {
    respond()->transferProtocolResponse(token, toAidl(resultCode));
    return {};
}

}  // namespace android::hardware::radio::compat
