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

#include "Fastboot.h"

using ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace fastboot {

ScopedAStatus Fastboot::getPartitionType(const std::string& in_partitionName, Result* out_result,
                                         FileSystemType* _aidl_return) {
    if (in_partitionName.empty()) {
        out_result->message = "Invalid partition name";
        out_result->status = Status::INVALID_ARGUMENT;
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Status::INVALID_ARGUMENT));
    }
    *_aidl_return = FileSystemType::RAW;
    out_result->message = "";
    out_result->status = Status::SUCCESS;
    return ScopedAStatus::ok();
}
ScopedAStatus Fastboot::doOemCommand(const std::string& in_oemCmd, Result* _aidl_return) {
    if (in_oemCmd.empty()) {
        _aidl_return->message = "Invalid command";
        _aidl_return->status = Status::INVALID_ARGUMENT;
        return ScopedAStatus::ok();
    }
    _aidl_return->status = Status::NOT_SUPPORTED;
    _aidl_return->message = "Command not supported in default implementation";
    return ScopedAStatus::ok();
}
ScopedAStatus Fastboot::getVariant(Result* out_result, std::string* _aidl_return) {
    *_aidl_return = "NA";
    out_result->status = Status::SUCCESS;
    out_result->message = "";
    return ScopedAStatus::ok();
}
ScopedAStatus Fastboot::getOffModeChargeState(Result* out_result, bool* _aidl_return) {
    *_aidl_return = false;
    out_result->status = Status::SUCCESS;
    out_result->message = "";
    return ScopedAStatus::ok();
}

ScopedAStatus Fastboot::getBatteryVoltageFlashingThreshold(Result* out_result,
                                                           int32_t* _aidl_return) {
    *_aidl_return = 0;
    out_result->status = Status::SUCCESS;
    out_result->message = "";
    return ScopedAStatus::ok();
}
ScopedAStatus Fastboot::doOemSpecificErase(Result* _aidl_return) {
    _aidl_return->status = Status::NOT_SUPPORTED;
    _aidl_return->message = "Command not supported in default implementation";
    return ScopedAStatus::ok();
}

}  // namespace fastboot
}  // namespace hardware
}  // namespace android
}  // namespace aidl
