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

#pragma once

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <aidl/android/hardware/audio/core/BnConfig.h>
#include <system/audio_config.h>

#include <android_audio_policy_capengine_configuration.h>
#include <android_audio_policy_capengine_configuration_enums.h>

#include "EngineConfigXmlConverter.h"

namespace aidl::android::hardware::audio::core::internal {

class CapEngineConfigXmlConverter {

public:
    explicit CapEngineConfigXmlConverter(const std::string& configFilePath)
            : mConverter(configFilePath,
                    &::android::audio::policy::capengine::configuration::readConfigurableDomains) {
        if (mConverter.getXsdcConfig()) {
            init();
        }
    }
    std::string getError() const { return mConverter.getError(); }
    ::android::status_t getStatus() const { return mConverter.getStatus(); }

    std::optional<std::vector<std::optional<
            ::aidl::android::media::audio::common::AudioHalCapDomain>>>& getAidlCapEngineConfig();

private:
    ConversionResult<::aidl::android::media::audio::common::AudioHalCapConfiguration>
            convertConfigurationToAidl(
            const ::android::audio::policy::capengine::configuration::ConfigurationsType::Configuration&
            xsdcConfiguration);
    ConversionResult<std::string> convertConfigurableElementToAidl(
            const ::android::audio::policy::capengine::configuration::ConfigurableElementsType::ConfigurableElement&
            xsdcConfigurableElement);
    ConversionResult<::aidl::android::media::audio::common::AudioHalCapSetting>
            convertSettingToAidl(
            const ::android::audio::policy::capengine::configuration::SettingsType::Configuration&
            xsdcSetting);
    ConversionResult<::aidl::android::media::audio::common::AudioHalCapDomain>
            convertConfigurableDomainToAidl(
            const ::android::audio::policy::capengine::configuration::ConfigurableDomainType&
            xsdcConfigurableDomain);


    const std::optional<::android::audio::policy::capengine::configuration::ConfigurableDomains>&
    getXsdcConfig() {
        return mConverter.getXsdcConfig();
    }
    void init();

    std::optional<std::vector<std::optional<
            ::aidl::android::media::audio::common::AudioHalCapDomain>>> mAidlCapDomains;
    XmlConverter<::android::audio::policy::capengine::configuration::ConfigurableDomains>
            mConverter;
};
}
