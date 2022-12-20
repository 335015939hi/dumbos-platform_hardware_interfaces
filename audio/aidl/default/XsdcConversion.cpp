#include <inttypes.h>

#include <unordered_set>

#define LOG_TAG "AHAL_Config"
#include <android-base/logging.h>
#include <android-base/strings.h>

#include <aidl/android/media/audio/common/AudioPort.h>
#include <media/AidlConversionCppNdk.h>
#include <media/TypeConverter.h>
#include <utils/Log.h>

#include "core-impl/XmlConverter.h"
#include "core-impl/XsdcConversion.h"

namespace ap_xsd = android::audio::policy::configuration;
namespace eng_xsd = android::audio::policy::engine::configuration;

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioGain;
using aidl::android::media::audio::common::AudioHalCapCriterion;
using aidl::android::media::audio::common::AudioHalCapCriterionType;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;

using ::android::BAD_VALUE;
using ::android::base::unexpected;

namespace aidl::android::hardware::audio::core::internal {

ConversionResult<AudioFormatDescription> convertAudioFormatToAidl(const std::string& xsdcFormat) {
    audio_format_t legacyFormat = ::android::formatFromString(xsdcFormat, AUDIO_FORMAT_DEFAULT);
    ConversionResult<AudioFormatDescription> result =
            legacy2aidl_audio_format_t_AudioFormatDescription(legacyFormat);
    if ((legacyFormat == AUDIO_FORMAT_DEFAULT && xsdcFormat.compare("AUDIO_FORMAT_DEFAULT") != 0) ||
        !result.ok()) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: " << xsdcFormat
                   << " is not a valid audio format.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioPort> convertDevicePortToAidl(
        const ap_xsd::DevicePorts::DevicePort& xDevicePort,
        const ap_xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    return AudioPort{
            .id = nextPortId++,
            .name = xDevicePort.getTagName(),
            .profiles = VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::Profile, AudioProfile>(
                    xDevicePort.getProfile(), convertAudioProfileToAidl))),
            .flags = VALUE_OR_FATAL(convertIoFlagsToAidl({}, xDevicePort.getRole(), false)),
            .gains = VALUE_OR_FATAL(
                    (convertWrappedCollectionToAidl<ap_xsd::Gains, ap_xsd::Gains::Gain, AudioGain>(
                            xDevicePort.getGains(), &ap_xsd::Gains::getGain, convertGainToAidl))),

            .ext = VALUE_OR_FATAL(createAudioPortExt(xDevicePort, xModuleConfig))};
}

ConversionResult<AudioPort> convertMixPortToAidl(const ap_xsd::MixPorts::MixPort& xMixPort,
                                                 int32_t& nextPortId) {
    return AudioPort{
            .id = nextPortId++,
            .name = xMixPort.getName(),
            .profiles = VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::Profile, AudioProfile>(
                    xMixPort.getProfile(), convertAudioProfileToAidl))),
            .flags = xMixPort.hasFlags()
                             ? VALUE_OR_FATAL(convertIoFlagsToAidl(xMixPort.getFlags(),
                                                                   xMixPort.getRole(), true))
                             : VALUE_OR_FATAL(convertIoFlagsToAidl({}, xMixPort.getRole(), true)),
            .gains = VALUE_OR_FATAL(
                    (convertWrappedCollectionToAidl<ap_xsd::Gains, ap_xsd::Gains::Gain, AudioGain>(
                            xMixPort.getGains(), &ap_xsd::Gains::getGain, &convertGainToAidl))),
            .ext = VALUE_OR_FATAL(createAudioPortExt(xMixPort)),
    };
}

ConversionResult<AudioProfile> convertAudioProfileToAidl(const ap_xsd::Profile& xProfile) {
    return AudioProfile{
            .name = xProfile.hasName() ? xProfile.getName() : "",
            .format = xProfile.hasFormat()
                              ? VALUE_OR_FATAL(convertAudioFormatToAidl(xProfile.getFormat()))
                              : AudioFormatDescription{},
            .channelMasks =
                    xProfile.hasChannelMasks()
                            ? VALUE_OR_FATAL((convertCollectionToAidl<ap_xsd::AudioChannelMask,
                                                                      AudioChannelLayout>(
                                      xProfile.getChannelMasks(), &convertChannelMaskToAidl)))
                            : std::vector<AudioChannelLayout>{},
            .sampleRates = xProfile.hasSamplingRates()
                                   ? VALUE_OR_FATAL((convertCollectionToAidl<int64_t, int>(
                                             xProfile.getSamplingRates(),
                                             [](const int64_t x) -> int { return x; })))
                                   : std::vector<int>{}};
}

ConversionResult<AudioHalCapCriterion> convertCapCriterionToAidl(
        const eng_xsd::CriterionType& xsdcCriterion) {
    AudioHalCapCriterion aidlCapCriterion;
    aidlCapCriterion.name = xsdcCriterion.getName();
    aidlCapCriterion.criterionTypeName = xsdcCriterion.getType();
    aidlCapCriterion.defaultLiteralValue = xsdcCriterion.get_default();
    return aidlCapCriterion;
}

ConversionResult<std::string> convertCriterionTypeValueToAidl(
        const eng_xsd::ValueType& xsdcCriterionTypeValue) {
    return xsdcCriterionTypeValue.getLiteral();
}

ConversionResult<AudioHalCapCriterionType> convertCapCriterionTypeToAidl(
        const eng_xsd::CriterionTypeType& xsdcCriterionType) {
    AudioHalCapCriterionType aidlCapCriterionType;
    aidlCapCriterionType.name = xsdcCriterionType.getName();
    aidlCapCriterionType.isInclusive = !(static_cast<bool>(xsdcCriterionType.getType()));
    aidlCapCriterionType.values = VALUE_OR_RETURN(
            (convertWrappedCollectionToAidl<eng_xsd::ValuesType, eng_xsd::ValueType, std::string>(
                    xsdcCriterionType.getValues(), &eng_xsd::ValuesType::getValue,
                    &convertCriterionTypeValueToAidl)));
    return aidlCapCriterionType;
}

ConversionResult<AudioChannelLayout> convertChannelMaskToAidl(
        const ap_xsd::AudioChannelMask& xChannelMask) {
    std::string xChannelMaskLiteral = ap_xsd::toString(xChannelMask);
    audio_channel_mask_t legacyChannelMask = ::android::channelMaskFromString(xChannelMaskLiteral);
    ConversionResult<AudioChannelLayout> result =
            legacy2aidl_audio_channel_mask_t_AudioChannelLayout(
                    legacyChannelMask,
                    /* isInput= */ xChannelMaskLiteral.find("AUDIO_CHANNEL_IN_") == 0);
    if ((legacyChannelMask == AUDIO_CHANNEL_INVALID) || !result.ok()) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: " << ap_xsd::toString(xChannelMask)
                   << " is not a valid audio channel mask.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioHalVolumeCurve::CurvePoint> convertCurvePointToAidl(
        const std::string& xsdcCurvePoint) {
    AudioHalVolumeCurve::CurvePoint aidlCurvePoint{};
    if ((sscanf(xsdcCurvePoint.c_str(), "%" SCNd8 ",%d", &aidlCurvePoint.index,
                &aidlCurvePoint.attenuationMb) != 2) ||
        (aidlCurvePoint.index < AudioHalVolumeCurve::CurvePoint::MIN_INDEX) ||
        (aidlCurvePoint.index > AudioHalVolumeCurve::CurvePoint::MAX_INDEX)) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: volume curve point:"
                   << "\"" << xsdcCurvePoint << "\" is invalid";
        return unexpected(BAD_VALUE);
    }
    return aidlCurvePoint;
}

ConversionResult<AudioDeviceDescription> convertDeviceTypeToAidl(const std::string& xType) {
    audio_devices_t legacyDeviceType = AUDIO_DEVICE_NONE;
    ::android::DeviceConverter::fromString(xType, legacyDeviceType);
    ConversionResult<AudioDeviceDescription> result =
            legacy2aidl_audio_devices_t_AudioDeviceDescription(legacyDeviceType);
    if ((legacyDeviceType == AUDIO_DEVICE_NONE) || !result.ok()) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: " << xType
                   << " is not a valid device type.";
        return unexpected(BAD_VALUE);
    }
    return result;
}

ConversionResult<AudioGain> convertGainToAidl(const ap_xsd::Gains::Gain& xGain) {
    return AudioGain{
            .mode = VALUE_OR_FATAL(convertGainModeToAidl(xGain.getMode())),
            .channelMask =
                    xGain.hasChannel_mask()
                            ? VALUE_OR_FATAL(convertChannelMaskToAidl(xGain.getChannel_mask()))
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

ConversionResult<int> convertGainModeToAidl(const std::vector<ap_xsd::AudioGainMode>& gainModeVec) {
    static const char gainModeSeparator = ' ';
    int gainModeMask = 0;
    for (const ap_xsd::AudioGainMode& gainMode : gainModeVec) {
        gainModeMask |= static_cast<int>(::android::GainModeConverter::maskFromString(
                ap_xsd::toString(gainMode), &gainModeSeparator));
    }
    return gainModeMask;
}

ConversionResult<AudioIoFlags> convertIoFlagsToAidl(
        const std::vector<ap_xsd::AudioInOutFlag>& flags, const ap_xsd::Role role,
        bool flagsForMixPort) {
    static const char flagSeparator = ' ';
    int flagMask = 0;
    if ((role == ap_xsd::Role::sink && flagsForMixPort) ||
        (role == ap_xsd::Role::source && !flagsForMixPort)) {
        for (const ap_xsd::AudioInOutFlag& flag : flags) {
            flagMask |= static_cast<int>(::android::InputFlagConverter::maskFromString(
                    ap_xsd::toString(flag), &flagSeparator));
        }
        return AudioIoFlags::make<AudioIoFlags::Tag::input>(flagMask);
    } else {
        for (const ap_xsd::AudioInOutFlag& flag : flags) {
            flagMask |= static_cast<int>(::android::OutputFlagConverter::maskFromString(
                    ap_xsd::toString(flag), &flagSeparator));
        }
    }
    return AudioIoFlags::make<AudioIoFlags::Tag::output>(flagMask);
}

ConversionResult<Module::Configuration> convertModuleConfigToAidl(
        const ap_xsd::Modules::Module& xModuleConfig) {
    Module::Configuration aidlModuleConfig;
    std::vector<AudioPort> devicePorts = VALUE_OR_FATAL(
            convertDevicePortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId));

    std::vector<AudioPort> mixPorts = VALUE_OR_FATAL(
            convertMixPortsInModuleToAidl(xModuleConfig, aidlModuleConfig.nextPortId));
    aidlModuleConfig.ports.reserve(devicePorts.size() + mixPorts.size());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), devicePorts.begin(),
                                  devicePorts.end());
    aidlModuleConfig.ports.insert(aidlModuleConfig.ports.end(), mixPorts.begin(), mixPorts.end());

    aidlModuleConfig.routes =
            VALUE_OR_FATAL(convertRoutesInModuleToAidl(xModuleConfig, aidlModuleConfig.ports));
    return aidlModuleConfig;
}

ConversionResult<std::vector<AudioRoute>> convertRoutesInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig,
        const std::vector<AudioPort>& aidlAudioPorts) {
    std::vector<AudioRoute> audioRouteVec;
    std::vector<ap_xsd::Routes> xRoutesVec = xModuleConfig.getRoutes();
    if (!xRoutesVec.empty()) {
        /*
         * xRoutesVec likely only contains one element; that is, it's
         * likely that all ap_xsd::Routes::MixPort types that we need to convert
         * are inside of xRoutesVec[0].
         */
        audioRouteVec.reserve(xRoutesVec[0].getRoute().size());
        for (const ap_xsd::Routes& xRoutesType : xRoutesVec) {
            for (const ap_xsd::Routes::Route& xRoute : xRoutesType.getRoute()) {
                audioRouteVec.push_back(VALUE_OR_FATAL(convertRouteToAidl(xRoute, aidlAudioPorts)));
            }
        }
    }
    return audioRouteVec;
}

ConversionResult<AudioRoute> convertRouteToAidl(const ap_xsd::Routes::Route& xRoute,
                                                const std::vector<AudioPort>& aidlAudioPorts) {
    std::unordered_map<std::string, int32_t> portMap;
    for (const AudioPort& port : aidlAudioPorts) {
        portMap.insert({port.name, port.id});
    }
    return AudioRoute{.sourcePortIds = VALUE_OR_FATAL(getSourcePortIds(xRoute, portMap)),
                      .sinkPortId = VALUE_OR_FATAL(getSinkPortId(xRoute, portMap)),
                      .isExclusive = (xRoute.getType() == ap_xsd::MixType::mux)};
}

ConversionResult<std::vector<AudioPort>> convertMixPortsInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<ap_xsd::MixPorts> xMixPortsVec = xModuleConfig.getMixPorts();
    if (!xMixPortsVec.empty()) {
        /*
         * xMixPortsVec likely only contains one element; that is, it's
         * likely that all ap_xsd::MixPorts::MixPort types that we need to convert
         * are inside of xMixPortsVec[0].
         */
        audioPortVec.reserve(xMixPortsVec[0].getMixPort().size());
        for (const ap_xsd::MixPorts& xMixPortsType : xMixPortsVec) {
            for (const ap_xsd::MixPorts::MixPort& xMixPort : xMixPortsType.getMixPort()) {
                audioPortVec.push_back(VALUE_OR_FATAL(convertMixPortToAidl(xMixPort, nextPortId)));
            }
        }
    }
    return audioPortVec;
}

ConversionResult<std::vector<AudioPort>> convertDevicePortsInModuleToAidl(
        const ap_xsd::Modules::Module& xModuleConfig, int32_t& nextPortId) {
    std::vector<AudioPort> audioPortVec;
    std::vector<ap_xsd::DevicePorts> xDevicePortsVec = xModuleConfig.getDevicePorts();
    if (!xDevicePortsVec.empty()) {
        /*
         * xDevicePortsVec likely only contains one element; that is, it's
         * likely that all ap_xsd::DevicePorts::DevicePort types that we need to convert
         * are inside of xDevicePortsVec[0].
         */
        audioPortVec.reserve(xDevicePortsVec[0].getDevicePort().size());
        for (const ap_xsd::DevicePorts& xDevicePortsType : xDevicePortsVec) {
            for (const ap_xsd::DevicePorts::DevicePort& xDevicePort :
                 xDevicePortsType.getDevicePort()) {
                audioPortVec.push_back(VALUE_OR_FATAL(
                        convertDevicePortToAidl(xDevicePort, xModuleConfig, nextPortId)));
            }
        }
    }
    return audioPortVec;
}

ConversionResult<AudioDevice> createAudioDevice(const ap_xsd::DevicePorts::DevicePort& xDevicePort,
                                                const ap_xsd::Modules::Module& xModuleConfig) {
    std::unordered_set<std::string> xAttachedDeviceSet = getAttachedDevices(xModuleConfig);
    AudioDevice device = {
            .type = VALUE_OR_FATAL(convertDeviceTypeToAidl(xDevicePort.getType())),
            .address = xDevicePort.hasAddress()
                               ? AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(
                                         xDevicePort.getAddress())
                               : AudioDeviceAddress{}};
    if (xAttachedDeviceSet.count(xDevicePort.getTagName()) != device.type.connection.empty()) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: <attachedDevices> "
                   << "list is incorrect or devicePort \"" << xDevicePort.getTagName()
                   << "\" type= " << xDevicePort.getType() << " is incorrect.";
        return unexpected(BAD_VALUE);
    }
    return device;
}

ConversionResult<AudioPortExt> createAudioPortExt(
        const ap_xsd::DevicePorts::DevicePort& xDevicePort,
        const ap_xsd::Modules::Module& xModuleConfig) {
    const std::string xDefaultOutputDevice =
            xModuleConfig.hasDefaultOutputDevice() ? xModuleConfig.getDefaultOutputDevice() : "";
    AudioPortDeviceExt deviceExt = {
            .device = VALUE_OR_FATAL(createAudioDevice(xDevicePort, xModuleConfig)),
            // TODO: check if it's preferred to check string equality a different way
            .flags = (xDevicePort.getTagName() == xDefaultOutputDevice)
                             ? 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE
                             : 0,
            .encodedFormats =
                    xDevicePort.hasEncodedFormats()
                            ? VALUE_OR_FATAL(
                                      (convertCollectionToAidl<std::string, AudioFormatDescription>(
                                              xDevicePort.getEncodedFormats(),
                                              &convertAudioFormatToAidl)))
                            : std::vector<AudioFormatDescription>{},
    };
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

ConversionResult<AudioPortExt> createAudioPortExt(const ap_xsd::MixPorts::MixPort& xMixPort) {
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

std::unordered_set<std::string> getAttachedDevices(const ap_xsd::Modules::Module& moduleConfig) {
    std::unordered_set<std::string> attachedDeviceSet;
    if (moduleConfig.hasAttachedDevices()) {
        for (const ap_xsd::AttachedDevices& attachedDevices : moduleConfig.getAttachedDevices()) {
            if (attachedDevices.hasItem()) {
                attachedDeviceSet.insert(attachedDevices.getItem().begin(),
                                         attachedDevices.getItem().end());
            }
        }
    }
    return attachedDeviceSet;
}

ConversionResult<int32_t> getSinkPortId(const ap_xsd::Routes::Route& xRoute,
                                        const std::unordered_map<std::string, int32_t>& portMap) {
    auto portMapIter = portMap.find(xRoute.getSink());
    if (portMapIter == portMap.end()) {
        LOG(ERROR) << __func__ << "Review Audio Policy config: audio route"
                   << "has sink: " << xRoute.getSink()
                   << " which is neither a device port nor mix port.";
        return unexpected(BAD_VALUE);
    }
    return portMapIter->second;
}

ConversionResult<std::vector<int32_t>> getSourcePortIds(
        const ap_xsd::Routes::Route& xRoute,
        const std::unordered_map<std::string, int32_t>& portMap) {
    std::vector<int32_t> sourcePortIds;
    for (std::string source : ::android::base::Split(xRoute.getSources(), ",")) {
        source = ::android::base::Trim(source);
        auto portMapIter = portMap.find(source);
        if (portMapIter == portMap.end()) {
            LOG(ERROR) << __func__ << "Review Audio Policy config: audio route"
                       << "has source: " << source
                       << " which is neither a device port nor mix port.";
            return unexpected(BAD_VALUE);
        }
        sourcePortIds.push_back(portMapIter->second);
    }
    return sourcePortIds;
}

}  // namespace aidl::android::hardware::audio::core::internal
