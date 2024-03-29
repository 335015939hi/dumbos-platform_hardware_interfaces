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

#include "core-impl/CapEngineConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioHalCapConfiguration;
using aidl::android::media::audio::common::AudioHalCapDomain;
using aidl::android::media::audio::common::AudioHalCapSetting;
using aidl::android::media::audio::common::AudioHalCapParameter;

using ::android::BAD_VALUE;
using ::android::base::unexpected;

namespace eng_xsd = android::audio::policy::capengine::configuration;

namespace aidl::android::hardware::audio::core::internal {

std::optional<std::vector<std::optional<AudioHalCapDomain>>>&
        CapEngineConfigXmlConverter::getAidlCapEngineConfig() {
    return mAidlCapDomains;
}

ConversionResult<std::string> convertRule(const eng_xsd::CompoundRuleType& xsdcCompoundRule)
{
    bool isPreviousCompoundRule = true;
    std::string rule;
    rule += eng_xsd::toString(xsdcCompoundRule.getType()) + "{";

    for (const auto& childXsdcCoumpoundRule : xsdcCompoundRule.getCompoundRule_optional()) {
        if (childXsdcCoumpoundRule.hasCompoundRule_optional()) {
            rule += (isPreviousCompoundRule ? "" : " , ");
            rule += VALUE_OR_FATAL(convertRule(childXsdcCoumpoundRule));
        } else if (childXsdcCoumpoundRule.hasSelectionCriterionRule_optional()) {
            rule += VALUE_OR_FATAL(convertRule(childXsdcCoumpoundRule));
        }
        isPreviousCompoundRule = false;
    }
    if (xsdcCompoundRule.hasSelectionCriterionRule_optional()) {
        for (const auto &criterionRule: xsdcCompoundRule.getSelectionCriterionRule_optional()) {
            rule += (isPreviousCompoundRule ? "" : " , ");
            isPreviousCompoundRule = false;
            std::string selectionCriterion = criterionRule.getSelectionCriterion();
            std::string matchesWhen = eng_xsd::toString(criterionRule.getMatchesWhen());
            std::string value = criterionRule.getValue();
            rule += " " + selectionCriterion + " " + matchesWhen + " " + value + " ";
        }
    }
    rule += "}";
    return rule;
}

ConversionResult<AudioHalCapConfiguration> CapEngineConfigXmlConverter::convertConfigurationToAidl(
        const eng_xsd::ConfigurationsType::Configuration& xsdcConfiguration) {
    AudioHalCapConfiguration aidlCapConfiguration;
    aidlCapConfiguration.name = xsdcConfiguration.getName();
    std::string rule;
    if (xsdcConfiguration.hasCompoundRule()) {
        for (const auto& compoundRule : xsdcConfiguration.getCompoundRule()) {
            rule += VALUE_OR_FATAL(convertRule(compoundRule));
        }
    }
    aidlCapConfiguration.rule = rule;
    return aidlCapConfiguration;
}

ConversionResult<std::string> CapEngineConfigXmlConverter::convertConfigurableElementToAidl(
        const eng_xsd::ConfigurableElementsType::ConfigurableElement& xsdcConfigurableElement) {
    return xsdcConfigurableElement.getPath();
}

template <typename ParamType>
ConversionResult<AudioHalCapSetting::ParameterSetting>
CapEngineConfigXmlConverter::convertParamToAidl(const ParamType& xsdcTypedParam,
        const eng_xsd::ConfigurableElementSettingsType& element) {
    const auto& path = element.getPath();
    const auto& name = xsdcTypedParam.getName();
    const auto& value = xsdcTypedParam.getValue();
    if (path.find(eng_xsd::toString(name)) == std::string::npos) {
        return unexpected(BAD_VALUE);
    }
    AudioHalCapParameter aidlParamName = static_cast<AudioHalCapParameter>(name);
    std::string xsdcNameLiteral = eng_xsd::toString(name);
    std::string aidlParamNameLiteral;
    std::transform(xsdcNameLiteral.begin(), xsdcNameLiteral.end(), xsdcNameLiteral.begin(),
                   [](char c) { return std::toupper(c); });
    if (xsdcNameLiteral != toString(aidlParamName)) {
        return unexpected(BAD_VALUE);
    }
    return AudioHalCapSetting::ParameterSetting{path, aidlParamName, value};
}

ConversionResult<AudioHalCapSetting> CapEngineConfigXmlConverter::convertSettingToAidl(
        const eng_xsd::SettingsType::Configuration& xsdcSetting) {
    AudioHalCapSetting aidlCapSetting;
    aidlCapSetting.configurationName = xsdcSetting.getName();
    for (const auto& element : xsdcSetting.getConfigurableElement()) {
        if (element.hasBooleanParameter_optional()) {
            const auto* xsdcParam =  element.getFirstBooleanParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasIntegerParameter_optional()) {
            const auto* xsdcParam = element.getFirstIntegerParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasEnumParameter_optional()) {
            const auto* xsdcParam = element.getFirstEnumParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasFixedPointParameter_optional()) {
            const auto* xsdcParam = element.getFirstFloatingPointParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasFloatingPointParameter_optional()) {
            const auto* xsdcParam = element.getFirstIntegerParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        }  else if (element.hasBitParameter_optional()) {
            const auto* xsdcParam = element.getFirstBitParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasBitParameterBlock_optional()) {
            const auto* xsdcParam = element.getFirstBitParameterBlock_optional();
            if (xsdcParam->hasBitParameter()) {
                const auto* xsdcBitParam = xsdcParam->getFirstBitParameter();
                const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcBitParam, element));
                aidlCapSetting.parameterSettings.push_back(param);
            } else {
                return unexpected(BAD_VALUE);
            }
        } else if (element.hasStringParameter_optional()) {
            const auto* xsdcParam = element.getFirstStringParameter_optional();
            const auto param = VALUE_OR_FATAL(convertParamToAidl(*xsdcParam, element));
            aidlCapSetting.parameterSettings.push_back(param);
        } else if (element.hasParameterBlock_optional()) {
            LOG(ERROR) << "Unsupported parameter block type used in settings.";
            return unexpected(BAD_VALUE);
        } else {
            LOG(ERROR) << "Unsupported parameter type used in settings.";
            return unexpected(BAD_VALUE);
        }
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
                        eng_xsd::ConfigurationsType::Configuration, AudioHalCapConfiguration>(
                        xsdcConfigurableDomain.getConfigurations(),
                        &eng_xsd::ConfigurationsType::getConfiguration,
                        std::bind(&CapEngineConfigXmlConverter::convertConfigurationToAidl, this,
                                  std::placeholders::_1))));
    }
    if (xsdcConfigurableDomain.hasConfigurableElements()) {
        aidlConfigurableDomain.parameterPaths = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::ConfigurableElementsType,
                        eng_xsd::ConfigurableElementsType::ConfigurableElement,
                        std::string>(
                        xsdcConfigurableDomain.getConfigurableElements(),
                        &eng_xsd::ConfigurableElementsType::getConfigurableElement,
                        std::bind(&CapEngineConfigXmlConverter::convertConfigurableElementToAidl,
                                  this, std::placeholders::_1))));
    }
    if (xsdcConfigurableDomain.hasSettings()) {
        aidlConfigurableDomain.capSettings = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::SettingsType,
                        eng_xsd::SettingsType::Configuration, AudioHalCapSetting>(
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

} // namespace android
