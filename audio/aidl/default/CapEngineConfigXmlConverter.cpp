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

#define LOG_TAG "AHAL_Config"

#include <aidl/android/media/audio/common/AudioProductStrategyType.h>
#include <android-base/logging.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>
#include <media/convert.h>

#include "core-impl/CapEngineConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioHalCapConfiguration;
using aidl::android::media::audio::common::AudioHalCapDomain;
using aidl::android::media::audio::common::AudioHalCapParameter;
using aidl::android::media::audio::common::AudioHalCapRule;
using aidl::android::media::audio::common::AudioHalCapSetting;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;

using ::android::BAD_VALUE;
using ::android::base::unexpected;
using ::android::utilities::convertTo;

namespace eng_xsd = android::audio::policy::capengine::configuration;

namespace aidl::android::hardware::audio::core::internal {

static constexpr const char *gStrategiesParameter = "product_strategies";
static constexpr const char *gInputSourcesParameter = "input_sources";
static constexpr const char *gStreamsParameter = "streams";
static constexpr const char *gOutputDevicesParameter = "selected_output_devices";
static constexpr const char *gOutputDeviceAddressParameter = "device_address";
static constexpr const char *gStrategyPrefix = "vx_";
static constexpr const char *gLegacyOutputDevicePrefix = "AUDIO_DEVICE_OUT_";
static constexpr const char *gLegacyInputDevicePrefix = "AUDIO_DEVICE_IN_";
static constexpr const char *gLegacyStreamPrefix = "AUDIO_STREAM_";
static constexpr const char *gLegacySourcePrefix = "AUDIO_SOURCE_";

std::optional<std::vector<std::optional<AudioHalCapDomain>>>&
CapEngineConfigXmlConverter::getAidlCapEngineConfig() {
    return mAidlCapDomains;
}
ConversionResult<AudioHalCapRule::CriterionRule> convertCriterionRule(
        const eng_xsd::SelectionCriterionRuleType& xsdcRule) {
    AudioHalCapRule::CriterionRule criterionRule{};
    criterionRule.criterionName = xsdcRule.getSelectionCriterion();
    criterionRule.criterionTypeValue = xsdcRule.getValue();
    if (xsdcRule.getMatchesWhen() == eng_xsd::MatchesWhenEnum::Excludes) {
        criterionRule.matchingRule = AudioHalCapRule::MatchingRule::EXCLUDES;
    } else if (xsdcRule.getMatchesWhen() == eng_xsd::MatchesWhenEnum::Includes) {
        criterionRule.matchingRule = AudioHalCapRule::MatchingRule::INCLUDES;
    } else if (xsdcRule.getMatchesWhen() == eng_xsd::MatchesWhenEnum::Is) {
        criterionRule.matchingRule = AudioHalCapRule::MatchingRule::IS;
    } else if (xsdcRule.getMatchesWhen() == eng_xsd::MatchesWhenEnum::IsNot) {
        criterionRule.matchingRule = AudioHalCapRule::MatchingRule::IS_NOT;
    } else {
        LOG(ERROR) << "Unsupported match when rule.";
        return unexpected(BAD_VALUE);
    }
    return criterionRule;
}

ConversionResult<AudioHalCapRule> convertRule(const eng_xsd::CompoundRuleType& xsdcCompoundRule) {
    AudioHalCapRule rule{};
    bool isPreviousCompoundRule = true;
    if (xsdcCompoundRule.getType() == eng_xsd::TypeEnum::Any) {
        rule.compoundRule = AudioHalCapRule::CompoundRule::ANY;
    } else if (xsdcCompoundRule.getType() == eng_xsd::TypeEnum::All) {
        rule.compoundRule = AudioHalCapRule::CompoundRule::ALL;
    } else {
        LOG(ERROR) << "Unsupported compound rule type.";
        return unexpected(BAD_VALUE);
    }
    std::vector<std::optional<AudioHalCapRule>> nestedRules;
    for (const auto& childXsdcCoumpoundRule : xsdcCompoundRule.getCompoundRule_optional()) {
        if (childXsdcCoumpoundRule.hasCompoundRule_optional()) {
            nestedRules.push_back(std::make_optional<AudioHalCapRule>(
                    std::move(VALUE_OR_FATAL(convertRule(childXsdcCoumpoundRule)))));
        } else if (childXsdcCoumpoundRule.hasSelectionCriterionRule_optional()) {
            nestedRules.push_back(std::make_optional<AudioHalCapRule>(
                    std::move(VALUE_OR_FATAL(convertRule(childXsdcCoumpoundRule)))));
        }
    }
    if (!nestedRules.empty()) {
        rule.nestedRules = std::make_optional<>(std::move(nestedRules));
    }
    if (xsdcCompoundRule.hasSelectionCriterionRule_optional()) {
        std::vector<std::optional<AudioHalCapRule::CriterionRule>> criterionRules{};
        for (const auto& xsdcRule : xsdcCompoundRule.getSelectionCriterionRule_optional()) {
            criterionRules.push_back(std::make_optional<AudioHalCapRule::CriterionRule>(
                    std::move(VALUE_OR_FATAL(convertCriterionRule(xsdcRule)))));
        }
        rule.criterionRules = std::make_optional<>(std::move(criterionRules));
    }
    return rule;
}

ConversionResult<AudioHalCapConfiguration> CapEngineConfigXmlConverter::convertConfigurationToAidl(
        const eng_xsd::ConfigurationsType::Configuration& xsdcConfiguration) {
    AudioHalCapConfiguration aidlCapConfiguration;
    aidlCapConfiguration.name = xsdcConfiguration.getName();
    if (xsdcConfiguration.hasCompoundRule()) {
        if (xsdcConfiguration.getCompoundRule().size() != 1) {
            return unexpected(BAD_VALUE);
        }
        aidlCapConfiguration.rule =
                VALUE_OR_FATAL(convertRule(xsdcConfiguration.getCompoundRule()[0]));
    }
    return aidlCapConfiguration;
}

ConversionResult<int> getAudioProductStrategyId(const std::string& path) {
    std::vector<std::string> strings;
    std::istringstream pathStream(path);
    std::string stringToken;
    while (getline(pathStream, stringToken, '/')) {
        std::size_t pos = stringToken.find(gStrategyPrefix);
        if (pos != std::string::npos) {
            std::string strategyIdLiteral = stringToken.substr(pos + std::strlen(gStrategyPrefix));
            int strategyId;
            if (!convertTo(strategyIdLiteral, strategyId)) {
                LOG(ERROR) << "Invalid strategy " << stringToken <<  " from path " << path;
                return unexpected(BAD_VALUE);
            }
            return strategyId;
        }
    }
    return unexpected(BAD_VALUE);
}

ConversionResult<AudioSource> getAudioSource(const std::string& path) {
    std::vector<std::string> strings;
    std::istringstream pathStream(path);
    std::string stringToken;
    while (getline(pathStream, stringToken, '/')) {
        if (stringToken.find(gInputSourcesParameter) != std::string::npos) {
            getline(pathStream, stringToken, '/');
            std::transform(stringToken.begin(), stringToken.end(), stringToken.begin(),
                           [](char c) { return std::toupper(c); });
            std::string legacySourceLiteral = "AUDIO_SOURCE_" + stringToken;
            audio_source_t legacySource;
            if (!::android::SourceTypeConverter::fromString(legacySourceLiteral, legacySource)) {
                LOG(ERROR) << "Invalid source " << stringToken <<  " from path " << path;
                return unexpected(BAD_VALUE);
            }
            return legacy2aidl_audio_source_t_AudioSource(legacySource);
        }
    }
    return unexpected(BAD_VALUE);
}

ConversionResult<AudioStreamType> getAudioStreamType(const std::string& path) {
    std::vector<std::string> strings;
    std::istringstream pathStream(path);
    std::string stringToken;

    while (getline(pathStream, stringToken, '/')) {
        if (stringToken.find(gStreamsParameter) != std::string::npos) {
            getline(pathStream, stringToken, '/');
            std::transform(stringToken.begin(), stringToken.end(), stringToken.begin(),
                           [](char c) { return std::toupper(c); });
            std::string legacyStreamLiteral = std::string(gLegacyStreamPrefix) + stringToken;
            audio_stream_type_t legacyStream;
            if (!::android::StreamTypeConverter::fromString(legacyStreamLiteral, legacyStream)) {
                LOG(ERROR) << "Invalid stream " << stringToken <<  " from path " << path;
                return unexpected(BAD_VALUE);
            }
            return legacy2aidl_audio_stream_type_t_AudioStreamType(legacyStream);
        }
    }
    return unexpected(BAD_VALUE);
}

ConversionResult<std::string> toUpperAndAppendPrefix(const std::string& capName,
        const std::string& legacyPrefix) {
    std::string legacyName = capName;
    std::transform(legacyName.begin(), legacyName.end(), legacyName.begin(),
                   [](char c) { return std::toupper(c); });
    return legacyPrefix + legacyName;
}

ConversionResult<AudioHalCapParameter> CapEngineConfigXmlConverter::convertParamToAidl(
        const eng_xsd::ConfigurableElementSettingsType& element) {
    const auto& path = element.getPath();

    AudioHalCapParameter parameterSetting;
    if (path.find(gStrategiesParameter) != std::string::npos) {
        int strategyId = VALUE_OR_FATAL(getAudioProductStrategyId(path));
        if (path.find(gOutputDevicesParameter) != std::string::npos) {
            // Value is 1 or 0
            if (!element.hasBitParameter_optional()) {
                LOG(ERROR) << "Invalid strategy value type";
                return unexpected(BAD_VALUE);
            }
            // Convert name to output device type
            const auto* xsdcParam = element.getFirstBitParameter_optional();
            std::string outputDevice = VALUE_OR_FATAL(toUpperAndAppendPrefix(
                    eng_xsd::toString(xsdcParam->getName()), gLegacyOutputDevicePrefix));
            audio_devices_t legacyType;
            if (!::android::OutputDeviceConverter::fromString(outputDevice, legacyType)) {
                LOG(ERROR) << "Invalid strategy device type " << outputDevice;
                return unexpected(BAD_VALUE);
            }
            AudioDeviceDescription aidlDevice = VALUE_OR_FATAL(
                    legacy2aidl_audio_devices_t_AudioDeviceDescription(legacyType));
            bool isSelected;
            if (!convertTo(xsdcParam->getValue(), isSelected)) {
                LOG(ERROR) << "Invalid strategy device selection value " << xsdcParam->getValue();
                return unexpected(BAD_VALUE);
            }
            parameterSetting = AudioHalCapParameter::StrategyDevice(aidlDevice,
                    strategyId, isSelected);
        } else if (path.find(gOutputDeviceAddressParameter) != std::string::npos) {
            // Value is the address
            if (!element.hasStringParameter_optional()) {
                return unexpected(BAD_VALUE);
            }
            std::string address = element.getFirstStringParameter_optional()->getValue();
            parameterSetting = AudioHalCapParameter::StrategyDeviceAddress(
                    AudioDeviceAddress(address), strategyId);
        }
    } else if (path.find(gInputSourcesParameter) != std::string::npos) {
        // Value is 1 or 0
        if (!element.hasBitParameter_optional()) {
            LOG(ERROR) << "Invalid source value type";
            return unexpected(BAD_VALUE);
        }
        AudioSource audioSourceAidl = VALUE_OR_FATAL(getAudioSource(path));
        const auto* xsdcParam = element.getFirstBitParameter_optional();
        std::string inputDeviceLiteral = VALUE_OR_FATAL(toUpperAndAppendPrefix(
                eng_xsd::toString(xsdcParam->getName()), gLegacyInputDevicePrefix));
        audio_devices_t inputDeviceType;
        if (!::android::InputDeviceConverter::fromString(inputDeviceLiteral, inputDeviceType)) {
            LOG(ERROR) << "Invalid source device type " << inputDeviceLiteral;
            return unexpected(BAD_VALUE);
        }
        AudioDeviceDescription aidlDevice = VALUE_OR_FATAL(
                legacy2aidl_audio_devices_t_AudioDeviceDescription(inputDeviceType));

        bool isSelected;
        if (!convertTo(xsdcParam->getValue(), isSelected)) {
            LOG(ERROR) << "Invalid source value type " << xsdcParam->getValue();
            return unexpected(BAD_VALUE);
        }
        parameterSetting = AudioHalCapParameter::InputSourceDevice(aidlDevice, audioSourceAidl,
                isSelected);
    } else if (path.find(gStreamsParameter) != std::string::npos) {
        AudioStreamType audioStreamAidl = VALUE_OR_FATAL(getAudioStreamType(path));
        if (!element.hasEnumParameter_optional()) {
            LOG(ERROR) << "Invalid stream value type";
            return unexpected(BAD_VALUE);
        }
        const auto* xsdcParam = element.getFirstEnumParameter_optional();
        std::string profileLiteral = VALUE_OR_FATAL(toUpperAndAppendPrefix(
                xsdcParam->getValue(), gLegacyStreamPrefix));
        audio_stream_type_t profileLegacyStream;
        if (!::android::StreamTypeConverter::fromString(profileLiteral, profileLegacyStream)) {
            LOG(ERROR) << "Invalid stream value " << profileLiteral;
            return unexpected(BAD_VALUE);
        }
        AudioStreamType profileStreamAidl = VALUE_OR_FATAL(
                legacy2aidl_audio_stream_type_t_AudioStreamType(profileLegacyStream));
        parameterSetting =
                AudioHalCapParameter::StreamVolumeProfile(audioStreamAidl, profileStreamAidl);
    }
    return parameterSetting;
}

ConversionResult<AudioHalCapSetting> CapEngineConfigXmlConverter::convertSettingToAidl(
        const eng_xsd::SettingsType::Configuration& xsdcSetting) {
    AudioHalCapSetting aidlCapSetting;
    aidlCapSetting.configurationName = xsdcSetting.getName();
    for (const auto& element : xsdcSetting.getConfigurableElement()) {
        aidlCapSetting.parameterSettings.push_back(VALUE_OR_FATAL(convertParamToAidl(element)));
    }
    return aidlCapSetting;
}

ConversionResult<AudioHalCapDomain> CapEngineConfigXmlConverter::convertConfigurableDomainToAidl(
        const eng_xsd::ConfigurableDomainType& xsdcConfigurableDomain) {
    AudioHalCapDomain aidlConfigurableDomain;

    aidlConfigurableDomain.name = xsdcConfigurableDomain.getName();
    if (xsdcConfigurableDomain.hasSequenceAware() && xsdcConfigurableDomain.getSequenceAware()) {
        LOG(ERROR) << "sequence aware not supported.";
        return unexpected(BAD_VALUE);
    }

    if (xsdcConfigurableDomain.hasConfigurations()) {
        aidlConfigurableDomain.configurations = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::ConfigurationsType,
                                                eng_xsd::ConfigurationsType::Configuration,
                                                AudioHalCapConfiguration>(
                        xsdcConfigurableDomain.getConfigurations(),
                        &eng_xsd::ConfigurationsType::getConfiguration,
                        std::bind(&CapEngineConfigXmlConverter::convertConfigurationToAidl, this,
                                  std::placeholders::_1))));
    }
    if (xsdcConfigurableDomain.hasSettings()) {
        aidlConfigurableDomain.capSettings =
                VALUE_OR_FATAL((convertWrappedCollectionToAidl<eng_xsd::SettingsType,
                                                               eng_xsd::SettingsType::Configuration,
                                                               AudioHalCapSetting>(
                        xsdcConfigurableDomain.getSettings(),
                        &eng_xsd::SettingsType::getConfiguration,
                        std::bind(&CapEngineConfigXmlConverter::convertSettingToAidl, this,
                                  std::placeholders::_1))));
    }
    return aidlConfigurableDomain;
}

void CapEngineConfigXmlConverter::init() {
    if (getXsdcConfig()->hasConfigurableDomain()) {
        mAidlCapDomains = std::make_optional<>(std::move(VALUE_OR_FATAL(
                (convertCollectionToAidlOptionalValues<eng_xsd::ConfigurableDomainType,
                                                       AudioHalCapDomain>(
                        getXsdcConfig()->getConfigurableDomain(),
                        std::bind(&CapEngineConfigXmlConverter::convertConfigurableDomainToAidl,
                                  this, std::placeholders::_1))))));
    } else {
        mAidlCapDomains = std::nullopt;
    }
}

}  // namespace aidl::android::hardware::audio::core::internal
