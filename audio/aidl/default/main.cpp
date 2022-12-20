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

#include <cstdlib>
#include <ctime>
#include <utility>
#include <vector>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "core-impl/AudioPolicyConfigXmlConverter.h"
#include "core-impl/ChildInterface.h"
#include "core-impl/Config.h"
#include "core-impl/ModuleBluetooth.h"
#include "core-impl/ModulePrimary.h"
#include "core-impl/ModuleRemoteSubmix.h"
#include "core-impl/ModuleStub.h"
#include "core-impl/ModuleUsb.h"

using aidl::android::hardware::audio::core::ChildInterface;
using aidl::android::hardware::audio::core::Config;
using aidl::android::hardware::audio::core::Module;
using aidl::android::hardware::audio::core::internal::AudioPolicyConfigXmlConverter;

std::shared_ptr<Module> makeModule(const std::string& name, Module::Configuration&& config) {
    if (name == "primary") {
        return ndk::SharedRefBase::make<aidl::android::hardware::audio::core::ModulePrimary>(
                name, std::move(config));
    } else if (name == "bluetooth") {
        return ndk::SharedRefBase::make<aidl::android::hardware::audio::core::ModuleBluetooth>(
                name, std::move(config));
    } else if (name == "r_submix") {
        return ndk::SharedRefBase::make<aidl::android::hardware::audio::core::ModuleRemoteSubmix>(
                name, std::move(config));
    } else if (name == "stub") {
        return ndk::SharedRefBase::make<aidl::android::hardware::audio::core::ModuleStub>(
                name, std::move(config));
    } else if (name == "usb") {
        return ndk::SharedRefBase::make<aidl::android::hardware::audio::core::ModuleUsb>(
                name, std::move(config));
    }
    LOG(ERROR) << __func__ << ": module type \"" << name << "\" is not supported";
    return nullptr;
}

ChildInterface<Module> createModule(const std::string& name, Module::Configuration&& config) {
    ChildInterface<Module> result;
    {
        auto module = makeModule(name, std::move(config));
        if (module == nullptr) return result;
        result = std::move(module);
    }
    // 'default' module is equivalent to 'primary' in the XML schema used by HIDL.
    const std::string moduleFqn = std::string()
                                          .append(Module::descriptor)
                                          .append("/")
                                          .append(name != "primary" ? name : "default");
    binder_status_t status = AServiceManager_addService(result.getBinder(), moduleFqn.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << __func__ << ": failed to register service for \"" << moduleFqn << "\"";
        return ChildInterface<Module>();
    }
    return result;
};

int main() {
    // Random values are used in the implementation.
    std::srand(std::time(nullptr));

    // This is a debug implementation, always enable debug logging.
    android::base::SetMinimumLogSeverity(::android::base::DEBUG);
    // For more logs, use VERBOSE, however this may hinder performance.
    // android::base::SetMinimumLogSeverity(::android::base::VERBOSE);
    ABinderProcess_setThreadPoolMaxThreadCount(16);

    // Guaranteed log for b/210919187 and logd_integration_test
    LOG(INFO) << "Init for Audio AIDL HAL";

    AudioPolicyConfigXmlConverter audioPolicyConverter{
            ::android::audio_get_audio_policy_config_file()};

    // Make the default config service
    auto config = ndk::SharedRefBase::make<Config>(audioPolicyConverter);
    const std::string configFqn = std::string().append(Config::descriptor).append("/default");
    binder_status_t status =
            AServiceManager_addService(config->asBinder().get(), configFqn.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "failed to register service for \"" << configFqn << "\"";
    }

    // Make modules
    std::vector<ChildInterface<Module>> moduleInstances;
    auto configs(audioPolicyConverter.releaseModuleConfigs());
    for (std::pair<std::string, Module::Configuration>& configPair : *configs) {
        std::string name = configPair.first;
        if (auto instance = createModule(name, std::move(configPair.second)); instance) {
            moduleInstances.push_back(std::move(instance));
        }
    }

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
