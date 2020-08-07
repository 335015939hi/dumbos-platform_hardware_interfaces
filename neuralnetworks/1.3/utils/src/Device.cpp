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

#include "Device.h"

#include "Buffer.h"
#include "Callbacks.h"
#include "Conversions.h"
#include "PreparedModel.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.1/Conversions.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

nn::GeneralResult<std::string> initVersionString(V1_3::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::string> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                            << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, const hidl_string& versionString) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "IDevice::getVersionString failed with " << toString(status);
        } else {
            result = versionString;
        }
    };

    const auto ret = device->getVersionString(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<nn::DeviceType> initDeviceType(V1_3::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<nn::DeviceType> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                               << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, V1_2::DeviceType deviceType) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "IDevice::getType failed with " << toString(status);
        } else {
            result = validatedConvertToCanonical(deviceType);
        }
    };

    const auto ret = device->getType(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<std::vector<nn::Extension>> initExtensions(V1_3::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::vector<nn::Extension>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status,
                              const hidl_vec<V1_2::Extension>& extensions) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "IDevice::getSupportedExtensions failed with " << toString(status);
        } else {
            result = validatedConvertToCanonical(extensions);
        }
    };

    const auto ret = device->getSupportedExtensions(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<nn::Capabilities> initCapabilities(V1_3::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<nn::Capabilities> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                 << "uninitialized";
    const auto cb = [&result](ErrorStatus status, const Capabilities& capabilities) {
        if (status != ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "IDevice::getCapabilities_1_3 failed with " << toString(status);
        } else {
            result = validatedConvertToCanonical(capabilities);
        }
    };

    const auto ret = device->getCapabilities_1_3(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<std::pair<uint32_t, uint32_t>> initNumberOfCacheFilesNeeded(
        V1_3::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::pair<uint32_t, uint32_t>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, uint32_t numModelCache,
                              uint32_t numDataCache) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "IDevice::getNumberOfCacheFilesNeeded failed with " << toString(status);
        } else {
            result = std::make_pair(numModelCache, numDataCache);
        }
    };

    const auto ret = device->getNumberOfCacheFilesNeeded(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<hidl_vec<sp<IPreparedModel>>> convert(
        const std::vector<std::shared_ptr<const nn::IPreparedModel>>& preparedModels) {
    hidl_vec<sp<IPreparedModel>> hidlPreparedModels(preparedModels.size());
    for (size_t i = 0; i < preparedModels.size(); ++i) {
        std::any underlyingResource = preparedModels[i]->getUnderlyingResource();
        if (const auto* preparedModel = std::any_cast<sp<PreparedModel>>(&underlyingResource)) {
            hidlPreparedModels[i] = (*preparedModel)->getHidlPreparedModel();
        } else {
            return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "";
        }
    }
    return hidlPreparedModels;
}

nn::GeneralResult<std::shared_ptr<const nn::IBuffer>> convert(
        nn::GeneralResult<std::shared_ptr<const Buffer>> result) {
    return NN_TRY(std::move(result));
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(std::string name,
                                                                sp<V1_3::IDevice> device) {
    CHECK(name != "");
    CHECK(device != nullptr);

    auto versionString = NN_TRY(initVersionString(device.get()));
    const auto deviceType = NN_TRY(initDeviceType(device.get()));
    auto extensions = NN_TRY(initExtensions(device.get()));
    auto capabilities = NN_TRY(initCapabilities(device.get()));
    const auto numberOfCacheFilesNeeded = NN_TRY(initNumberOfCacheFilesNeeded(device.get()));

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(device));
    return std::make_shared<const Device>(
            PrivateConstructorTag{}, std::move(name), std::move(versionString), deviceType,
            std::move(extensions), std::move(capabilities), numberOfCacheFilesNeeded,
            std::move(device), std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, std::string versionString,
               nn::DeviceType deviceType, std::vector<nn::Extension> extensions,
               nn::Capabilities capabilities,
               std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded, sp<V1_3::IDevice> device,
               hal::utils::DeathHandler deathHandler)
    : kName(std::move(name)),
      kVersionString(std::move(versionString)),
      kDeviceType(deviceType),
      kExtensions(std::move(extensions)),
      kCapabilities(std::move(capabilities)),
      kNumberOfCacheFilesNeeded(numberOfCacheFilesNeeded),
      kDevice(std::move(device)),
      kDeathHandler(std::move(deathHandler)) {}

const std::string& Device::getName() const {
    return kName;
}

const std::string& Device::getVersionString() const {
    return kVersionString;
}

nn::Version Device::getFeatureLevel() const {
    return nn::Version::ANDROID_R;
}

nn::DeviceType Device::getType() const {
    return kDeviceType;
}

const std::vector<nn::Extension>& Device::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& Device::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> Device::getNumberOfCacheFilesNeeded() const {
    return kNumberOfCacheFilesNeeded;
}

nn::GeneralResult<void> Device::wait() const {
    const auto ret = kDevice->ping();
    return hal::utils::handleTransportError(ret);
}

nn::GeneralResult<std::vector<bool>> Device::getSupportedOperations(const nn::Model& model) const {
    // Ensure that model is ready for IPC.
    const bool alreadyOptimized = hal::utils::hasNoPointerData(model);
    std::optional<nn::Model> maybeModelInShared;
    if (!alreadyOptimized) {
        maybeModelInShared = NN_TRY(hal::utils::flushDataFromPointerToShared(model));
    }
    const nn::Model& modelInShared = alreadyOptimized ? model : maybeModelInShared.value();

    const auto hidlModel = NN_TRY(convert(modelInShared));
    // TODO: handle model partition in case where full model is not supported.

    nn::GeneralResult<std::vector<bool>> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                  << "uninitialized";
    auto cb = [&result, &model](ErrorStatus status, const hidl_vec<bool>& supportedOperations) {
        if (status != ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "IDevice::getSupportedOperations_1_3 failed with " << toString(status);
        } else if (supportedOperations.size() != model.main.operations.size()) {
            result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                     << "IDevice::getSupportedOperations_1_3 returned vector of size "
                     << supportedOperations.size() << " but expected "
                     << model.main.operations.size();
        } else {
            result = supportedOperations;
        }
    };

    const auto ret = kDevice->getSupportedOperations_1_3(hidlModel, cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    // Ensure that model is ready for IPC.
    const bool alreadyInShared = hal::utils::hasNoPointerData(model);
    std::optional<nn::Model> maybeModelInShared;
    if (!alreadyInShared) {
        maybeModelInShared = NN_TRY(hal::utils::flushDataFromPointerToShared(model));
    }
    const nn::Model& modelInShared = alreadyInShared ? model : maybeModelInShared.value();

    const auto hidlModel = NN_TRY(convert(modelInShared));
    const auto hidlPreference = NN_TRY(V1_1::utils::convert(preference));
    const auto hidlPriority = NN_TRY(convert(priority));
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlModelCache = NN_TRY(V1_2::utils::convert(modelCache));
    const auto hidlDataCache = NN_TRY(V1_2::utils::convert(dataCache));
    const auto hidlToken = token;

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret =
            kDevice->prepareModel_1_3(hidlModel, hidlPreference, hidlPriority, hidlDeadline,
                                      hidlModelCache, hidlDataCache, hidlToken, cb);
    const auto status = NN_TRY(hal::utils::handleTransportError(ret));
    if (status != ErrorStatus::NONE) {
        const auto canonical =
                validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "prepareModel_1_3 failed with " << toString(status);
    }

    return cb->get();
}

nn::GeneralResult<std::shared_ptr<const nn::IPreparedModel>> Device::prepareModelFromCache(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlModelCache = NN_TRY(V1_2::utils::convert(modelCache));
    const auto hidlDataCache = NN_TRY(V1_2::utils::convert(dataCache));
    const auto hidlToken = token;

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModelFromCache_1_3(hidlDeadline, hidlModelCache, hidlDataCache,
                                                        hidlToken, cb);
    const auto status = NN_TRY(hal::utils::handleTransportError(ret));
    if (status != ErrorStatus::NONE) {
        const auto canonical =
                validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "prepareModelFromCache_1_3 failed with " << toString(status);
    }

    return cb->get();
}

nn::GeneralResult<std::shared_ptr<const nn::IBuffer>> Device::allocate(
        const nn::BufferDesc& desc,
        const std::vector<std::shared_ptr<const nn::IPreparedModel>>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    const auto hidlDesc = NN_TRY(convert(desc));
    const auto hidlPreparedModels = NN_TRY(convert(preparedModels));
    const auto hidlInputRoles = NN_TRY(convert(inputRoles));
    const auto hidlOutputRoles = NN_TRY(convert(outputRoles));

    nn::GeneralResult<std::shared_ptr<const nn::IBuffer>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    auto cb = [&result](ErrorStatus status, const sp<IBuffer>& buffer, uint32_t token) {
        if (status != ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "IDevice::allocate failed with " << toString(status);
        } else if (buffer == nullptr) {
            result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Returned buffer is nullptr";
        } else if (token == 0) {
            result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Returned token is invalid (0)";
        } else {
            result = convert(
                    Buffer::create(buffer, static_cast<nn::Request::MemoryDomainToken>(token)));
        }
    };

    const auto ret =
            kDevice->allocate(hidlDesc, hidlPreparedModels, hidlInputRoles, hidlOutputRoles, cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
