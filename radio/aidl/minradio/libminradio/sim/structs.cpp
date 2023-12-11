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
#include <libminradio/sim/structs.h>

#include <libminradio/sim/IccConstants.h>

namespace android::hardware::radio::minimal::structs {

using namespace sim::icc::constants;
namespace aidl = ::aidl::android::hardware::radio::sim;

aidl::IccIoResult toIccIoResult(std::string_view simResponse) {
    return {.sw1 = IO_RESULT_SUCCESS, .simResponse = std::string(simResponse)};
}

aidl::IccIoResult toIccIoResult(std::pair<int, int> pair) {
    return {.sw1 = pair.first, .sw2 = pair.second};
}

std::ostream& operator<<(std::ostream& os, const aidl::IccIo& iccIo) {
    os << "IccIo{"                                //
       << "command=" << iccIo.command             //
       << " fileId=" << std::hex << iccIo.fileId  //
       << " path=" << iccIo.path                  //
       << " p1=" << iccIo.p1                      //
       << " p2=" << iccIo.p2                      //
       << " p3=" << iccIo.p3                      //
       << " data=" << iccIo.data;
    if (!iccIo.pin2.empty()) os << " pin2=?";
    return os << " aid=" << iccIo.aid << "}";
}

std::ostream& operator<<(std::ostream& os, const aidl::SessionInfo& sessionInfo) {
    return os << "SessionInfo{"                   //
              << "id=" << sessionInfo.sessionId   //
              << "isEs10=" << sessionInfo.isEs10  //
              << "}";
}

}  // namespace android::hardware::radio::minimal::structs
