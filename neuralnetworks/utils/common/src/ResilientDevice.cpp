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

#include "ResilientDevice.h"

#include "ResilientBuffer.h"
#include "ResilientPreparedModel.h"

#include <android-base/logging.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientDevice& resilientDevice, const FnType& fn, bool blocking) {
    auto device = resilientDevice.getDevice();
    auto result = fn(*device);

    // Immediately return if device is not dead.
    if (result.has_value() /* || result.error().code != nn::ErrorStatus::DEAD_OBJECT*/) {
        return result;
    }

    device = resilientDevice.recover(device.get(), blocking);
    return fn(*device);
}

bool equal(const std::vector<nn::Extension::OperandTypeInformation>& lhs,
           const std::vector<nn::Extension::OperandTypeInformation>& rhs) {
    constexpr auto cmp = [](const nn::Extension::OperandTypeInformation& lhs,
                            const nn::Extension::OperandTypeInformation& rhs) {
        return lhs.type == rhs.type && lhs.isTensor == rhs.isTensor && lhs.byteSize == rhs.byteSize;
    };
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), cmp);
}

bool equal(const std::vector<nn::Extension>& lhs, const std::vector<nn::Extension>& rhs) {
    constexpr auto cmp = [](const nn::Extension& lhs, const nn::Extension& rhs) {
        return lhs.name == rhs.name && equal(lhs.operandTypes, rhs.operandTypes);
    };
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), cmp);
}

bool equal(const nn::Capabilities::PerformanceInfo& lhs,
           const nn::Capabilities::PerformanceInfo& rhs) {
    return lhs.execTime == rhs.execTime && lhs.powerUsage == rhs.powerUsage;
}

bool equal(const std::vector<nn::Capabilities::OperandPerformance>& lhs,
           const std::vector<nn::Capabilities::OperandPerformance>& rhs) {
    constexpr auto cmp = [](const nn::Capabilities::OperandPerformance& lhs,
                            const nn::Capabilities::OperandPerformance& rhs) {
        return lhs.type == rhs.type && equal(lhs.info, rhs.info);
    };
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), cmp);
}

bool equal(const nn::Capabilities& lhs, const nn::Capabilities& rhs) {
    return equal(lhs.relaxedFloat32toFloat16PerformanceScalar,
                 rhs.relaxedFloat32toFloat16PerformanceScalar) &&
           equal(lhs.relaxedFloat32toFloat16PerformanceTensor,
                 rhs.relaxedFloat32toFloat16PerformanceTensor) &&
           equal(lhs.ifPerformance, rhs.ifPerformance) &&
           equal(lhs.whilePerformance, rhs.whilePerformance) &&
           equal(lhs.operandPerformance.asVector(), rhs.operandPerformance.asVector());
}

}  // namespace

std::shared_ptr<const nn::IDevice> ResilientDevice::create(Factory makeDevice) {
    CHECK(makeDevice != nullptr);
    auto device = makeDevice(/*blocking=*/true);
    CHECK(device != nullptr);

    auto name = device->getName();
    auto versionString = device->getVersionString();
    auto extensions = device->getSupportedExtensions();
    auto capabilities = device->getCapabilities();

    return std::make_shared<ResilientDevice>(PrivateConstructorTag{}, std::move(makeDevice),
                                             std::move(name), std::move(versionString),
                                             std::move(extensions), std::move(capabilities),
                                             std::move(device));
}

ResilientDevice::ResilientDevice(PrivateConstructorTag /*tag*/, Factory makeDevice,
                                 std::string name, std::string versionString,
                                 std::vector<nn::Extension> extensions,
                                 nn::Capabilities capabilities,
                                 std::shared_ptr<const nn::IDevice> device)
    : kMakeDevice(std::move(makeDevice)),
      kName(std::move(name)),
      kVersionString(std::move(versionString)),
      kExtensions(std::move(extensions)),
      kCapabilities(std::move(capabilities)),
      mDevice(std::move(device)) {
    CHECK(kMakeDevice != nullptr);
    CHECK(mDevice != nullptr);
}

std::shared_ptr<const nn::IDevice> ResilientDevice::getDevice() const {
    std::lock_guard guard(mMutex);
    return mDevice;
}

std::shared_ptr<const nn::IDevice> ResilientDevice::recover(const nn::IDevice* failingDevice,
                                                            bool blocking) const {
    std::lock_guard guard(mMutex);
    if (mDevice.get() == failingDevice) {
        if (auto device = kMakeDevice(blocking)) {
            mDevice = std::move(device);

            CHECK(kName == mDevice->getName());
            CHECK(kVersionString == mDevice->getVersionString());
            CHECK(equal(kExtensions, mDevice->getSupportedExtensions()));
            CHECK(equal(kCapabilities, mDevice->getCapabilities()));
        }
    }
    return mDevice;
}

const std::string& ResilientDevice::getName() const {
    return kName;
}

const std::string& ResilientDevice::getVersionString() const {
    return kVersionString;
}

nn::Version ResilientDevice::getFeatureLevel() const {
    return getDevice()->getFeatureLevel();
}

nn::DeviceType ResilientDevice::getType() const {
    return getDevice()->getType();
}

const std::vector<nn::Extension>& ResilientDevice::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& ResilientDevice::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> ResilientDevice::getNumberOfCacheFilesNeeded() const {
    return getDevice()->getNumberOfCacheFilesNeeded();
}

nn::GeneralResult<void> ResilientDevice::wait() const {
    const auto fn = [](const nn::IDevice& device) { return device.wait(); };
    return protect(*this, fn, /*blocking=*/true);
}

nn::GeneralResult<std::vector<bool>> ResilientDevice::getSupportedOperations(
        const nn::Model& model) const {
    const auto fn = [&model](const nn::IDevice& device) {
        return device.getSupportedOperations(model);
    };
    return protect(*this, fn, /*blocking=*/false);
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>> ResilientDevice::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    auto self = shared_from_this();
    ResilientPreparedModel::Factory makePreparedModel =
            [self = std::move(self), model, preference, priority, deadline, modelCache, dataCache,
             token](bool blocking) -> std::shared_ptr<const nn::IPreparedModel> {
        return self
                ->prepareModelInternal(blocking, model, preference, priority, deadline, modelCache,
                                       dataCache, token)
                .value_or(nullptr);
    };
    return ResilientPreparedModel::create(std::move(makePreparedModel));
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>> ResilientDevice::prepareModelFromCache(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    auto self = shared_from_this();
    ResilientPreparedModel::Factory makePreparedModel =
            [self = std::move(self), deadline, modelCache, dataCache,
             token](bool blocking) -> std::shared_ptr<const nn::IPreparedModel> {
        return self->prepareModelFromCacheInternal(blocking, deadline, modelCache, dataCache, token)
                .value_or(nullptr);
    };
    return ResilientPreparedModel::create(std::move(makePreparedModel));
}

nn::GeneralResult<std::shared_ptr<const nn::IBuffer>> ResilientDevice::allocate(
        const nn::BufferDesc& desc,
        const std::vector<std::shared_ptr<const nn::IPreparedModel>>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    auto self = shared_from_this();
    ResilientBuffer::Factory makeBuffer =
            [self = std::move(self), desc, preparedModels, inputRoles,
             outputRoles](bool blocking) -> std::shared_ptr<const nn::IBuffer> {
        return self->allocateInternal(blocking, desc, preparedModels, inputRoles, outputRoles)
                .value_or(nullptr);
    };
    return ResilientBuffer::create(std::move(makeBuffer));
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>> ResilientDevice::prepareModelInternal(
        bool blocking, const nn::Model& model, nn::ExecutionPreference preference,
        nn::Priority priority, nn::OptionalTimePoint deadline,
        const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    const auto fn = [&model, preference, priority, deadline, &modelCache, &dataCache,
                     token](const nn::IDevice& device) {
        return device.prepareModel(model, preference, priority, deadline, modelCache, dataCache,
                                   token);
    };
    return protect(*this, fn, blocking);
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>>
ResilientDevice::prepareModelFromCacheInternal(bool blocking, nn::OptionalTimePoint deadline,
                                               const std::vector<nn::SharedHandle>& modelCache,
                                               const std::vector<nn::SharedHandle>& dataCache,
                                               const nn::CacheToken& token) const {
    const auto fn = [deadline, &modelCache, &dataCache, token](const nn::IDevice& device) {
        return device.prepareModelFromCache(deadline, modelCache, dataCache, token);
    };
    return protect(*this, fn, blocking);
}

nn::GeneralResult<std::shared_ptr<const nn::IBuffer>> ResilientDevice::allocateInternal(
        bool blocking, const nn::BufferDesc& desc,
        const std::vector<std::shared_ptr<const nn::IPreparedModel>>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    const auto fn = [&desc, &preparedModels, &inputRoles, &outputRoles](const nn::IDevice& device) {
        return device.allocate(desc, preparedModels, inputRoles, outputRoles);
    };
    return protect(*this, fn, blocking);
}

}  // namespace android::hardware::neuralnetworks::utils
