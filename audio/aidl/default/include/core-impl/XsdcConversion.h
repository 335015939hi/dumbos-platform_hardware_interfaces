#include <string>
#include <unordered_map>
#include <unordered_set>

#include <aidl/android/media/audio/common/AudioHalCapCriterion.h>
#include <aidl/android/media/audio/common/AudioHalCapCriterionType.h>
#include <aidl/android/media/audio/common/AudioHalVolumeCurve.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <android_audio_policy_configuration.h>
#include <android_audio_policy_configuration_enums.h>
#include <android_audio_policy_engine_configuration.h>
#include <media/AidlConversionUtil.h>

#include "core-impl/Module.h"

namespace aidl::android::hardware::audio::core::internal {

ConversionResult<::aidl::android::media::audio::common::AudioFormatDescription>
convertAudioFormatToAidl(const std::string& xsdcFormat);
ConversionResult<::aidl::android::media::audio::common::AudioPort> convertDevicePortToAidl(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
ConversionResult<::aidl::android::media::audio::common::AudioPort> convertMixPortToAidl(
        const ::android::audio::policy::configuration::MixPorts::MixPort& xMixPort);
ConversionResult<::aidl::android::media::audio::common::AudioProfile> convertAudioProfileToAidl(
        const ::android::audio::policy::configuration::Profile& xProfile);
ConversionResult<::aidl::android::media::audio::common::AudioHalCapCriterion>
convertCapCriterionToAidl(
        const ::android::audio::policy::engine::configuration::CriterionType& xsdcCriterion);
ConversionResult<::aidl::android::media::audio::common::AudioHalCapCriterionType>
convertCapCriterionTypeToAidl(
        const ::android::audio::policy::engine::configuration::CriterionTypeType&
                xsdcCriterionType);
ConversionResult<std::string> convertCriterionTypeValueToAidl(
        const ::android::audio::policy::engine::configuration::ValueType& xsdcCriterionTypeValue);
ConversionResult<::aidl::android::media::audio::common::AudioChannelLayout>
convertChannelMaskToAidl(
        const ::android::audio::policy::configuration::AudioChannelMask& xChannelMask);
ConversionResult<::aidl::android::media::audio::common::AudioHalVolumeCurve::CurvePoint>
convertCurvePointToAidl(const std::string& xsdcCurvePoint);
ConversionResult<::aidl::android::media::audio::common::AudioDeviceDescription>
convertDeviceTypeToAidl(const std::string& xType,
                        const ::android::audio::policy::configuration::Role& role);
ConversionResult<::aidl::android::media::audio::common::AudioGain> convertGainToAidl(
        const ::android::audio::policy::configuration::Gains::Gain& xGain);
ConversionResult<int> convertGainModeToAidl(
        const std::vector<::android::audio::policy::configuration::AudioGainMode>& gainModeVec);
ConversionResult<::aidl::android::media::audio::common::AudioIoFlags> convertIoFlagsToAidl(
        const std::vector<::android::audio::policy::configuration::AudioInOutFlag>& flags,
        const ::android::audio::policy::configuration::Role role, bool flagsForMixPort);
ConversionResult<Module::Configuration> convertModuleConfigToAidl(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
ConversionResult<std::vector<::aidl::android::media::audio::common::AudioPort>>
convertDevicePortsInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        int32_t& nextPortId);
ConversionResult<std::vector<::aidl::android::media::audio::common::AudioPort>>
convertMixPortsInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        int32_t& nextPortId);
ConversionResult<AudioRoute> convertRouteToAidl(
        const ::android::audio::policy::configuration::Routes::Route& xRoute,
        const std::vector<::aidl::android::media::audio::common::AudioPort>& aidlAudioPorts);
ConversionResult<std::vector<AudioRoute>> convertRoutesInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        const std::vector<::aidl::android::media::audio::common::AudioPort>& aidlAudioPorts);
ConversionResult<::aidl::android::media::audio::common::AudioDevice> createAudioDevice(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
ConversionResult<::aidl::android::media::audio::common::AudioPortExt> createAudioPortExt(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
ConversionResult<::aidl::android::media::audio::common::AudioPortExt> createAudioPortExt(
        const ::android::audio::policy::configuration::MixPorts::MixPort& xMixPort);
std::unordered_set<std::string> getAttachedDevices(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
ConversionResult<int32_t> getSinkPortId(
        const ::android::audio::policy::configuration::Routes::Route& xRoute,
        const std::unordered_map<std::string, int32_t>& portMap);
ConversionResult<std::vector<int32_t>> getSourcePortIds(
        const ::android::audio::policy::configuration::Routes::Route& xRoute,
        const std::unordered_map<std::string, int32_t>& portMap);
}  // namespace aidl::android::hardware::audio::core::internal
