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
#include "radio_sap_utils.h"

#include <android-base/logging.h>

RadioSapResponse::RadioSapResponse(RadioSapTest& parent) : parent_sap(parent) {}

::ndk::ScopedAStatus RadioSapResponse::apduResponse(int32_t token, SapResultCode resultCode,
                                                    const std::vector<uint8_t>& /*apduRsp*/) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus RadioSapResponse::connectResponse(int32_t token,
                                                       SapConnectRsp /*sapConnectRsp*/,
                                                       int32_t /*maxMsgSize*/) {
    sapResponseToken = token;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::disconnectIndication(
        int32_t /*token*/, SapDisconnectType /*sapDisconnectType*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::disconnectResponse(int32_t token) {
    sapResponseToken = token;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::errorResponse(int32_t /*token*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::powerResponse(int32_t token, SapResultCode resultCode) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::resetSimResponse(int32_t token, SapResultCode resultCode) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::statusIndication(int32_t /*token*/,
                                                        SapStatus /*sapStatus*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::transferAtrResponse(int32_t token, SapResultCode resultCode,
                                                           const std::vector<uint8_t>& /*atr*/) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::transferCardReaderStatusResponse(
        int32_t token, SapResultCode resultCode, int32_t /*cardReaderStatus*/) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus RadioSapResponse::transferProtocolResponse(int32_t token,
                                                                SapResultCode resultCode) {
    sapResponseToken = token;
    sapResultCode = resultCode;
    parent_sap.notify(token);
    return ndk::ScopedAStatus::ok();
}
