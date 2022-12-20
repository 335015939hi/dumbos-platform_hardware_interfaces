#include <unordered_set>

#define LOG_TAG "AHAL_Config"
#include <android-base/logging.h>
#include <android-base/strings.h>

#include <aidl/android/media/audio/common/AudioPort.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>

#include "core-impl/XmlConverter.h"
#include "core-impl/XsdcConversion.h"

namespace xsd = android::audio::policy::configuration;

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGain;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;

namespace aidl::android::hardware::audio::core::internal {

AudioFormatDescription convertAudioFormatToAidl(const std::string& xsdcFormat) {
    audio_format_t legacyFormat = ::android::formatFromString(xsdcFormat, AUDIO_FORMAT_DEFAULT);
    ConversionResult<AudioFormatDescription> result =
            legacy2aidl_audio_format_t_AudioFormatDescription(legacyFormat);
    if ((legacyFormat == AUDIO_FORMAT_DEFAULT && xsdcFormat.compare("AUDIO_FORMAT_DEFAULT") != 0) ||
        !result.ok()) {
        LOG(WARNING) << __func__ << "Review Audio Policy config: " << xsdcFormat
                     << " is not a valid audio format.";
        return AudioFormatDescription{.type = AudioFormatType::DEFAULT};
    }
    return result.value();
}

AudioPort convertDevicePortToAidl(const xsd::DevicePorts::DevicePort& xDevicePort,
                                  const xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    return {.id = nextPortId++,
            .name = xDevicePort.getTagName(),
            .profiles = convertCollectionToAidl<xsd::Profile, AudioProfile>(
                    xDevicePort.getProfile(), convertAudioProfileToAidl),
            .flags = convertIoFlagsToAidl({}, xDevicePort.getRole(), false),
            .gains = convertWrappedCollectionToAidl<xsd::Gains, xsd::Gains::Gain, AudioGain>(
                    xDevicePort.getGains(), &xsd::Gains::getGain, &convertGainToAidl),

            .ext = createAudioPortExt(xDevicePort, xModuleConfig)};
}

AudioPort convertMixPortToAidl(const xsd::MixPorts::MixPort& xMixPort, int32_t& nextPortId) {
    return {
            .id = nextPortId++,
            .name = xMixPort.getName(),
            .profiles = convertCollectionToAidl<xsd::Profile, AudioProfile>(
                    xMixPort.getProfile(), convertAudioProfileToAidl),
            .flags = xMixPort.hasFlags()
                             ? convertIoFlagsToAidl(xMixPort.getFlags(), xMixPort.getRole(), true)
                             : convertIoFlagsToAidl({}, xMixPort.getRole(), true),
            .gains = convertWrappedCollectionToAidl<xsd::Gains, xsd::Gains::Gain, AudioGain>(
                    xMixPort.getGains(), &xsd::Gains::getGain, &convertGainToAidl),
            .ext = createAudioPortExt(xMixPort),
    };
}

AudioProfile convertAudioProfileToAidl(const xsd::Profile& xProfile) {
    return {.name = xProfile.hasName() ? xProfile.getName() : "",
            .format = xProfile.hasFormat() ? convertAudioFormatToAidl(xProfile.getFormat())
                                           : AudioFormatDescription{},
            .channelMasks =
                    xProfile.hasChannelMasks()
                            ? convertCollectionToAidl<xsd::AudioChannelMask, AudioChannelLayout>(
                                      xProfile.getChannelMasks(), &convertChannelMaskToAidl)
                            : std::vector<AudioChannelLayout>{},
            .sampleRates = xProfile.hasSamplingRates()
                                   ? convertCollectionToAidl<int64_t, int>(
                                             xProfile.getSamplingRates(),
                                             [](const int64_t x) -> int { return x; })
                                   : std::vector<int>{}};
}

AudioChannelLayout convertChannelMaskToAidl(const xsd::AudioChannelMask& xChannelMask) {
    audio_channel_mask_t legacyChannelMask =
            ::android::channelMaskFromString(xsd::toString(xChannelMask));
    // TODO: fix second param
    ConversionResult<AudioChannelLayout> result =
            legacy2aidl_audio_channel_mask_t_AudioChannelLayout(legacyChannelMask, false);
    if ((legacyChannelMask == AUDIO_CHANNEL_INVALID) || !result.ok()) {
        LOG(WARNING) << __func__ << "Review Audio Policy config: " << xsd::toString(xChannelMask)
                     << " is not a valid audio channel mask.";
        return AudioChannelLayout::make<AudioChannelLayout::Tag::invalid>(0);
    }
    return result.value();
}

AudioDeviceDescription convertDeviceTypeToAidl(const std::string& xType) {
    audio_devices_t legacyDeviceType = AUDIO_DEVICE_NONE;
    ::android::DeviceConverter::fromString(xType, legacyDeviceType);
    ConversionResult<AudioDeviceDescription> result =
            legacy2aidl_audio_devices_t_AudioDeviceDescription(legacyDeviceType);
    if ((legacyDeviceType == AUDIO_DEVICE_NONE) || !result.ok()) {
        LOG(WARNING) << __func__ << "Review Audio Policy config: " << xType
                     << " is not a valid device type.";
        return AudioDeviceDescription{};
    }
    return result.value();
}

AudioGain convertGainToAidl(const xsd::Gains::Gain& xGain) {
    return {
            .mode = convertGainModeToAidl(xGain.getMode()),
            .channelMask = xGain.hasChannel_mask()
                                   ? convertChannelMaskToAidl(xGain.getChannel_mask())
                                   : AudioChannelLayout{},
            .minValue = xGain.hasMinValueMB() ? xGain.getMinValueMB() : 0,
            .maxValue = xGain.hasMaxValueMB() ? xGain.getMaxValueMB() : 0,
            .defaultValue = xGain.hasDefaultValueMB() ? xGain.getDefaultValueMB() : 0,
            .stepValue = xGain.hasStepValueMB() ? xGain.getStepValueMB() : 0,
            .minRampMs = xGain.hasMinRampMs() ? xGain.getMinRampMs() : 0,
            .maxRampMs = xGain.hasMaxRampMs() ? xGain.getMaxRampMs() : 0,
            .useForVolume = xGain.hasUseForVolume() ? xGain.getUseForVolume() : false,
    };
}

int convertGainModeToAidl(const std::vector<xsd::AudioGainMode>& gainModeVec) {
    static const char gainModeSeparator = ' ';
    int gainModeMask = 0;
    for (const xsd::AudioGainMode& gainMode : gainModeVec) {
        gainModeMask |= static_cast<int>(::android::GainModeConverter::maskFromString(
                xsd::toString(gainMode), &gainModeSeparator));
    }
    return gainModeMask;
}

AudioIoFlags convertIoFlagsToAidl(const std::vector<xsd::AudioInOutFlag>& flags,
                                  const xsd::Role role, bool flagsForMixPort) {
    static const char flagSeparator = ' ';
    int flagMask = 0;
    if ((role == xsd::Role::sink && flagsForMixPort) ||
        (role == xsd::Role::source && !flagsForMixPort)) {
        for (const xsd::AudioInOutFlag& flag : flags) {
            flagMask |= static_cast<int>(::android::InputFlagConverter::maskFromString(
                    xsd::toString(flag), &flagSeparator));
        }
        return AudioIoFlags::make<AudioIoFlags::Tag::input>(flagMask);
    } else {
        for (const xsd::AudioInOutFlag& flag : flags) {
            flagMask |= static_cast<int>(::android::OutputFlagConverter::maskFromString(
                    xsd::toString(flag), &flagSeparator));
        }
    }
    return AudioIoFlags::make<AudioIoFlags::Tag::output>(flagMask);
}

Configuration convertModuleConfigToAidl(const xsd::Modules::Module& xModuleConfig) {
    Configuration aidlModuleConfig;
    std::vector<AudioPort> devicePorts =
            convertDevicePortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId);

    std::vector<AudioPort> mixPorts =
            convertMixPortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId);
    aidlModuleConfig.ports.reserve(devicePorts.size() + mixPorts.size());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), devicePorts.begin(),
                                  devicePorts.end());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), mixPorts.begin(), mixPorts.end());

    aidlModuleConfig.routes = convertRoutesInModuleToAidl(xModuleConfig, aidlModuleConfig.ports);
    return aidlModuleConfig;
}

std::vector<AudioRoute> convertRoutesInModuleToAidl(const xsd::Modules::Module& xModuleConfig,
                                                    const std::vector<AudioPort>& aidlAudioPorts) {
    std::vector<AudioRoute> audioRouteVec;
    std::vector<xsd::Routes> xRoutesVec = xModuleConfig.getRoutes();
    if (!xRoutesVec.empty()) {
        /*
         * xRoutesVec likely only contains one element; that is, it's
         * likely that all xsd::Routes::MixPort types that we need to convert
         * are inside of xRoutesVec[0].
         */
        audioRouteVec.reserve(xRoutesVec[0].getRoute().size());
        for (const xsd::Routes& xRoutesType : xRoutesVec) {
            for (const xsd::Routes::Route& xRoute : xRoutesType.getRoute()) {
                audioRouteVec.push_back(convertRouteToAidl(xRoute, aidlAudioPorts));
            }
        }
    }
    return audioRouteVec;
}

AudioRoute convertRouteToAidl(const xsd::Routes::Route& xRoute,
                              const std::vector<AudioPort>& aidlAudioPorts) {
    std::unordered_map<std::string, int32_t> portMap;
    for (const AudioPort& port : aidlAudioPorts) {
        portMap.insert({port.name, port.id});
    }
    return AudioRoute{.sourcePortIds = getSourcePortIds(xRoute, portMap),
                      .sinkPortId = getSinkPortId(xRoute, portMap),
                      .isExclusive = (xRoute.getType() == xsd::MixType::mux)};
}

std::vector<AudioPort> convertMixPortsInModuleToAidl(const xsd::Modules::Module& xModuleConfig,
                                                     int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<xsd::MixPorts> xMixPortsVec = xModuleConfig.getMixPorts();
    if (!xMixPortsVec.empty()) {
        /*
         * xMixPortsVec likely only contains one element; that is, it's
         * likely that all xsd::MixPorts::MixPort types that we need to convert
         * are inside of xMixPortsVec[0].
         */
        audioPortVec.reserve(xMixPortsVec[0].getMixPort().size());
        for (const xsd::MixPorts& xMixPortsType : xMixPortsVec) {
            for (const xsd::MixPorts::MixPort& xMixPort : xMixPortsType.getMixPort()) {
                audioPortVec.push_back(convertMixPortToAidl(xMixPort, nextPortId));
            }
        }
    }
    return audioPortVec;
}

std::vector<AudioPort> convertDevicePortsInModuleToAidl(const xsd::Modules::Module& xModuleConfig,
                                                        int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<xsd::DevicePorts> xDevicePortsVec = xModuleConfig.getDevicePorts();
    if (!xDevicePortsVec.empty()) {
        /*
         * xDevicePortsVec likely only contains one element; that is, it's
         * likely that all xsd::DevicePorts::DevicePort types that we need to convert
         * are inside of xDevicePortsVec[0].
         */
        audioPortVec.reserve(xDevicePortsVec[0].getDevicePort().size());
        for (const xsd::DevicePorts& xDevicePortsType : xDevicePortsVec) {
            for (const xsd::DevicePorts::DevicePort& xDevicePort :
                 xDevicePortsType.getDevicePort()) {
                audioPortVec.push_back(
                        convertDevicePortToAidl(xDevicePort, xModuleConfig, nextPortId));
            }
        }
    }
    return audioPortVec;
}

AudioDevice createAudioDevice(const xsd::DevicePorts::DevicePort& xDevicePort,
                              const xsd::Modules::Module& xModuleConfig) {
    std::unordered_set<std::string> xAttachedDeviceSet = getAttachedDevices(xModuleConfig);
    AudioDevice device = {
            .type = convertDeviceTypeToAidl(xDevicePort.getType()),
            .address = xDevicePort.hasAddress()
                               ? AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(
                                         xDevicePort.getAddress())
                               : AudioDeviceAddress{}};
    if (xAttachedDeviceSet.count(xDevicePort.getTagName()) != device.type.connection.empty()) {
        LOG(WARNING) << __func__ << "Review Audio Policy config: <attachedDevices> "
                     << "list is incorrect or devicePort \"" << xDevicePort.getTagName()
                     << "\" type= " << xDevicePort.getType() << " is incorrect.";
    }
    return device;
}

AudioPortExt createAudioPortExt(const xsd::DevicePorts::DevicePort& xDevicePort,
                                const xsd::Modules::Module& xModuleConfig) {
    const std::string xDefaultOutputDevice =
            xModuleConfig.hasDefaultOutputDevice() ? xModuleConfig.getDefaultOutputDevice() : "";
    AudioPortDeviceExt deviceExt = {
            .device = createAudioDevice(xDevicePort, xModuleConfig),
            .flags = (xDevicePort.getTagName() == xDefaultOutputDevice)
                             ? 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE
                             : 0,
            .encodedFormats =
                    xDevicePort.hasEncodedFormats()
                            ? convertCollectionToAidl<std::string, AudioFormatDescription>(
                                      xDevicePort.getEncodedFormats(), &convertAudioFormatToAidl)
                            : std::vector<AudioFormatDescription>{},
    };
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

AudioPortExt createAudioPortExt(const xsd::MixPorts::MixPort& xMixPort) {
    AudioPortMixExt mixExt = {
            .maxOpenStreamCount =
                    xMixPort.hasMaxOpenCount() ? static_cast<int>(xMixPort.getMaxOpenCount()) : 0,
            .maxActiveStreamCount = xMixPort.hasMaxActiveCount()
                                            ? static_cast<int>(xMixPort.getMaxActiveCount())
                                            : 0,
            .recommendedMuteDurationMs =
                    xMixPort.hasRecommendedMuteDurationMs()
                            ? static_cast<int>(xMixPort.getRecommendedMuteDurationMs())
                            : 0};
    return AudioPortExt::make<AudioPortExt::Tag::mix>(mixExt);
}

std::unordered_set<std::string> getAttachedDevices(const xsd::Modules::Module& moduleConfig) {
    std::unordered_set<std::string> attachedDeviceSet;
    if (moduleConfig.hasAttachedDevices()) {
        for (const xsd::AttachedDevices& attachedDevices : moduleConfig.getAttachedDevices()) {
            if (attachedDevices.hasItem()) {
                attachedDeviceSet.insert(attachedDevices.getItem().begin(),
                                         attachedDevices.getItem().end());
            }
        }
    }
    return attachedDeviceSet;
}

int32_t getSinkPortId(const xsd::Routes::Route& xRoute,
                      const std::unordered_map<std::string, int32_t>& portMap) {
    auto portMapIter = portMap.find(xRoute.getSink());
    if (portMapIter == portMap.end()) {
        LOG(WARNING) << __func__ << "Review Audio Policy config: audio route"
                     << "has sink: " << xRoute.getSink()
                     << " which is neither a device port nor mix port.";
    }
    return portMapIter->second;
}
std::vector<int32_t> getSourcePortIds(const xsd::Routes::Route& xRoute,
                                      const std::unordered_map<std::string, int32_t>& portMap) {
    std::vector<int32_t> sourcePortIds;
    for (std::string source : ::android::base::Split(xRoute.getSources(), ",")) {
        source = ::android::base::Trim(source);
        auto portMapIter = portMap.find(source);
        if (portMapIter == portMap.end()) {
            LOG(WARNING) << __func__ << "Review Audio Policy config: audio route"
                         << "has source: " << source
                         << " which is neither a device port nor mix port.";
        }
        sourcePortIds.push_back(portMapIter->second);
    }
    return sourcePortIds;
}
}  // namespace aidl::android::hardware::audio::core::internal
