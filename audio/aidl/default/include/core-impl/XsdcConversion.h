#include <string>
#include <unordered_map>
#include <unordered_set>

#include <aidl/android/media/audio/common/AudioPort.h>
#include <android_audio_policy_configuration.h>
#include <android_audio_policy_configuration_enums.h>

#include "core-impl/Configuration.h"

namespace aidl::android::hardware::audio::core::internal {

::aidl::android::media::audio::common::AudioFormatDescription convertAudioFormatToAidl(
        const std::string& xsdcFormat);
::aidl::android::media::audio::common::AudioPort convertDevicePortToAidl(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
::aidl::android::media::audio::common::AudioPort convertMixPortToAidl(
        const ::android::audio::policy::configuration::MixPorts::MixPort& xMixPort);
::aidl::android::media::audio::common::AudioProfile convertAudioProfileToAidl(
        const ::android::audio::policy::configuration::Profile& xProfile);
::aidl::android::media::audio::common::AudioChannelLayout convertChannelMaskToAidl(
        const ::android::audio::policy::configuration::AudioChannelMask& xChannelMask);
::aidl::android::media::audio::common::AudioDeviceDescription convertDeviceTypeToAidl(
        const std::string& xType, const ::android::audio::policy::configuration::Role& role);
::aidl::android::media::audio::common::AudioGain convertGainToAidl(
        const ::android::audio::policy::configuration::Gains::Gain& xGain);
int convertGainModeToAidl(
        const std::vector<::android::audio::policy::configuration::AudioGainMode>& gainModeVec);
::aidl::android::media::audio::common::AudioIoFlags convertIoFlagsToAidl(
        const std::vector<::android::audio::policy::configuration::AudioInOutFlag>& flags,
        const ::android::audio::policy::configuration::Role role, bool flagsForMixPort);
Configuration convertModuleConfigToAidl(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
std::vector<::aidl::android::media::audio::common::AudioPort> convertDevicePortsInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        int32_t& nextPortId);
std::vector<::aidl::android::media::audio::common::AudioPort> convertMixPortsInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        int32_t& nextPortId);
AudioRoute convertRouteToAidl(
        const ::android::audio::policy::configuration::Routes::Route& xRoute,
        const std::vector<::aidl::android::media::audio::common::AudioPort>& aidlAudioPorts);
std::vector<AudioRoute> convertRoutesInModuleToAidl(
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig,
        const std::vector<::aidl::android::media::audio::common::AudioPort>& aidlAudioPorts);
::aidl::android::media::audio::common::AudioDevice createAudioDevice(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
::aidl::android::media::audio::common::AudioPortExt createAudioPortExt(
        const ::android::audio::policy::configuration::DevicePorts::DevicePort& xDevicePort,
        const ::android::audio::policy::configuration::Modules::Module& xModuleConfig);
::aidl::android::media::audio::common::AudioPortExt createAudioPortExt(
        const ::android::audio::policy::configuration::MixPorts::MixPort& xMixPort);
std::unordered_set<std::string> getAttachedDevices(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
int32_t getSinkPortId(const ::android::audio::policy::configuration::Routes::Route& xRoute,
                      const std::unordered_map<std::string, int32_t>& portMap);
std::vector<int32_t> getSourcePortIds(
        const ::android::audio::policy::configuration::Routes::Route& xRoute,
        const std::unordered_map<std::string, int32_t>& portMap);
}  // namespace aidl::android::hardware::audio::core::internal
