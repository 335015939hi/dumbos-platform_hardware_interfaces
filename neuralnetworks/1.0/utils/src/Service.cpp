/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Service.h"

#include <android-base/logging.h>
#include <nnapi/IDevice.h>
#include <nnapi/Result.h>
#include <memory>
#include <string>
#include "Device.h"

namespace android::hardware::neuralnetworks::V1_0::utils {

std::shared_ptr<const nn::IDevice> getDevice(const std::string& name) {
    auto service = IDevice::getService(name);
    auto maybeDevice = Device::create(name, std::move(service));
    if (!maybeDevice.has_value()) {
        LOG(ERROR) << "V1_0::utils::getDevice failed: " << maybeDevice.error().message;
        return nullptr;
    }
    return std::move(maybeDevice).value();
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
