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

#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>

#include <functional>
#include <unordered_map>

#define LOG_TAG "AHAL_ApmXmlConverter"
#include <android-base/logging.h>

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <system/audio-base-utils.h>

#include "core-impl/AidlConversionXsdc.h"
#include "core-impl/AudioPolicyConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortDeviceExt;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioPortMixExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::PcmType;

namespace ap_xsd = android::audio::policy::configuration;

namespace aidl::android::hardware::audio::core::internal {

namespace {

void fillProfile(AudioProfile* profile, const std::vector<int32_t>& channelLayouts,
                 const std::vector<int32_t>& sampleRates) {
    for (auto layout : channelLayouts) {
        profile->channelMasks.push_back(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout));
    }
    profile->sampleRates.insert(profile->sampleRates.end(), sampleRates.begin(), sampleRates.end());
}

AudioPortExt createDeviceExt(AudioDeviceType devType, int32_t flags, std::string connection = "") {
    AudioPortDeviceExt deviceExt;
    deviceExt.device.type.type = devType;
    deviceExt.device.type.connection = std::move(connection);
    deviceExt.flags = flags;
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

AudioPortExt createPortMixExt(int32_t maxOpenStreamCount, int32_t maxActiveStreamCount) {
    AudioPortMixExt mixExt;
    mixExt.maxOpenStreamCount = maxOpenStreamCount;
    mixExt.maxActiveStreamCount = maxActiveStreamCount;
    return AudioPortExt::make<AudioPortExt::Tag::mix>(mixExt);
}

AudioPort createPort(int32_t id, const std::string& name, int32_t flags, bool isInput,
                     const AudioPortExt& ext) {
    AudioPort port;
    port.id = id;
    port.name = name;
    port.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                         : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    port.ext = ext;
    return port;
}

AudioProfile createProfile(PcmType pcmType, const std::vector<int32_t>& channelLayouts,
                           const std::vector<int32_t>& sampleRates) {
    AudioProfile profile;
    profile.format.type = AudioFormatType::PCM;
    profile.format.pcm = pcmType;
    fillProfile(&profile, channelLayouts, sampleRates);
    return profile;
}

AudioRoute createRoute(const std::vector<AudioPort>& sources, const AudioPort& sink) {
    AudioRoute route;
    route.sinkPortId = sink.id;
    std::transform(sources.begin(), sources.end(), std::back_inserter(route.sourcePortIds),
                   [](const auto& port) { return port.id; });
    return route;
}

// Note: There are several reasons not to parse the remote submix configuration from XML:
//   1. The "Remote Submix In" device is listed as "attached", and this is not correct
//      in the new of things where both remote submix input and output devices have
//      a "virtual" connection.
//   2. The canonical r_submix configuration only lists 'STEREO' and '48000',
//      however the framework attempts to open streams for other sample rates
//      as well. The legacy r_submix implementation allowed that, but libaudiohal@aidl
//      will not find a mix port to use. Because of that, list all channel
//      masks and sample rates that the legacy implementation allowed.
//   3. The legacy implementation had a hard limit on the number of routes (10),
//      and this is checked indirectly by AudioPlaybackCaptureTest#testPlaybackCaptureDoS
//      CTS test. Instead of hardcoding the number of routes, we can use
//      "maxOpen/ActiveStreamCount" to enforce a similar limit. However, the canonical
//      XML file lacks this specification.

Module::Configuration getStandardRemoteSubmixConfiguration() {
    Module::Configuration c;
    const std::vector<AudioProfile> standardPcmAudioProfiles{
            createProfile(PcmType::INT_16_BIT,
                          {AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_STEREO},
                          {8000, 11025, 16000, 32000, 44100, 48000})};

    // Device ports

    AudioPort rsubmixOutDevice =
            createPort(c.nextPortId++, "Remote Submix Out", 0, false,
                       createDeviceExt(AudioDeviceType::OUT_SUBMIX, 0,
                                       AudioDeviceDescription::CONNECTION_VIRTUAL));
    rsubmixOutDevice.profiles = standardPcmAudioProfiles;
    c.ports.push_back(rsubmixOutDevice);

    AudioPort rsubmixInDevice =
            createPort(c.nextPortId++, "Remote Submix In", 0, true,
                       createDeviceExt(AudioDeviceType::IN_SUBMIX, 0,
                                       AudioDeviceDescription::CONNECTION_VIRTUAL));
    rsubmixInDevice.profiles = standardPcmAudioProfiles;
    c.ports.push_back(rsubmixInDevice);

    // Mix ports

    AudioPort rsubmixOutMix =
            createPort(c.nextPortId++, "r_submix output", 0, false, createPortMixExt(20, 10));
    rsubmixOutMix.profiles = standardPcmAudioProfiles;
    c.ports.push_back(rsubmixOutMix);

    AudioPort rsubmixInMix =
            createPort(c.nextPortId++, "r_submix input", 0, true, createPortMixExt(20, 10));
    rsubmixInMix.profiles = standardPcmAudioProfiles;
    c.ports.push_back(rsubmixInMix);

    c.routes.push_back(createRoute({rsubmixOutMix}, rsubmixOutDevice));
    c.routes.push_back(createRoute({rsubmixInDevice}, rsubmixInMix));

    return c;
}

}  // namespace

static const int kDefaultVolumeIndexMin = 0;
static const int kDefaultVolumeIndexMax = 100;
static const int KVolumeIndexDeferredToAudioService = -1;

ConversionResult<AudioHalVolumeCurve> AudioPolicyConfigXmlConverter::convertVolumeCurveToAidl(
        const ap_xsd::Volume& xsdcVolumeCurve) {
    AudioHalVolumeCurve aidlVolumeCurve;
    aidlVolumeCurve.deviceCategory =
            static_cast<AudioHalVolumeCurve::DeviceCategory>(xsdcVolumeCurve.getDeviceCategory());
    if (xsdcVolumeCurve.hasRef()) {
        if (mVolumesReferenceMap.empty()) {
            mVolumesReferenceMap = generateReferenceMap<ap_xsd::Volumes, ap_xsd::Reference>(
                    getXsdcConfig()->getVolumes());
        }
        aidlVolumeCurve.curvePoints = VALUE_OR_FATAL(
                (convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        mVolumesReferenceMap.at(xsdcVolumeCurve.getRef()).getPoint(),
                        &convertCurvePointToAidl)));
    } else {
        aidlVolumeCurve.curvePoints = VALUE_OR_FATAL(
                (convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        xsdcVolumeCurve.getPoint(), &convertCurvePointToAidl)));
    }
    return aidlVolumeCurve;
}

void AudioPolicyConfigXmlConverter::mapStreamToVolumeCurve(const ap_xsd::Volume& xsdcVolumeCurve) {
    mStreamToVolumeCurvesMap[xsdcVolumeCurve.getStream()].push_back(
            VALUE_OR_FATAL(convertVolumeCurveToAidl(xsdcVolumeCurve)));
}

const SurroundSoundConfig& AudioPolicyConfigXmlConverter::getSurroundSoundConfig() {
    static const SurroundSoundConfig aidlSurroundSoundConfig = [this]() {
        if (auto xsdcConfig = getXsdcConfig(); xsdcConfig && xsdcConfig->hasSurroundSound()) {
            auto configConv = xsdc2aidl_SurroundSoundConfig(*xsdcConfig->getFirstSurroundSound());
            if (configConv.ok()) {
                return configConv.value();
            }
            LOG(ERROR) << "There was an error converting surround formats to AIDL: "
                       << configConv.error();
        }
        LOG(WARNING) << "Audio policy config does not have <surroundSound> section, using default";
        return getDefaultSurroundSoundConfig();
    }();
    return aidlSurroundSoundConfig;
}

std::unique_ptr<AudioPolicyConfigXmlConverter::ModuleConfigs>
AudioPolicyConfigXmlConverter::releaseModuleConfigs() {
    return std::move(mModuleConfigurations);
}

const AudioHalEngineConfig& AudioPolicyConfigXmlConverter::getAidlEngineConfig() {
    if (mAidlEngineConfig.volumeGroups.empty() && getXsdcConfig() &&
        getXsdcConfig()->hasVolumes()) {
        parseVolumes();
    }
    return mAidlEngineConfig;
}

// static
const SurroundSoundConfig& AudioPolicyConfigXmlConverter::getDefaultSurroundSoundConfig() {
    // Provide a config similar to the one used by the framework by default
    // (see AudioPolicyConfig::setDefaultSurroundFormats).
#define ENCODED_FORMAT(format)        \
    AudioFormatDescription {          \
        .encoding = ::android::format \
    }
#define SIMPLE_FORMAT(format)                   \
    SurroundSoundConfig::SurroundFormatFamily { \
        .primaryFormat = ENCODED_FORMAT(format) \
    }

    static const SurroundSoundConfig defaultConfig = {
            .formatFamilies = {
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_AC3),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_EAC3),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_HD),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_HD_MA),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_UHD_P1),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_UHD_P2),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DOLBY_TRUEHD),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_EAC3_JOC),
                    SurroundSoundConfig::SurroundFormatFamily{
                            .primaryFormat = ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_LC),
                            .subFormats =
                                    {
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_HE_V1),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_HE_V2),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_ELD),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_XHE),
                                    }},
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_AC4),
            }};
#undef SIMPLE_FORMAT
#undef ENCODED_FORMAT

    return defaultConfig;
}

void AudioPolicyConfigXmlConverter::mapStreamsToVolumeCurves() {
    if (getXsdcConfig()->hasVolumes()) {
        for (const ap_xsd::Volumes& xsdcWrapperType : getXsdcConfig()->getVolumes()) {
            for (const ap_xsd::Volume& xsdcVolume : xsdcWrapperType.getVolume()) {
                mapStreamToVolumeCurve(xsdcVolume);
            }
        }
    }
}

void AudioPolicyConfigXmlConverter::addVolumeGroupstoEngineConfig() {
    for (const auto& [xsdcStream, volumeCurves] : mStreamToVolumeCurvesMap) {
        AudioHalVolumeGroup volumeGroup;
        volumeGroup.name = ap_xsd::toString(xsdcStream);
        if (static_cast<int>(xsdcStream) >= AUDIO_STREAM_PUBLIC_CNT) {
            volumeGroup.minIndex = kDefaultVolumeIndexMin;
            volumeGroup.maxIndex = kDefaultVolumeIndexMax;
        } else {
            volumeGroup.minIndex = KVolumeIndexDeferredToAudioService;
            volumeGroup.maxIndex = KVolumeIndexDeferredToAudioService;
        }
        volumeGroup.volumeCurves = volumeCurves;
        mAidlEngineConfig.volumeGroups.push_back(std::move(volumeGroup));
    }
}

void AudioPolicyConfigXmlConverter::parseVolumes() {
    if (mStreamToVolumeCurvesMap.empty() && getXsdcConfig()->hasVolumes()) {
        mapStreamsToVolumeCurves();
        addVolumeGroupstoEngineConfig();
    }
}

void AudioPolicyConfigXmlConverter::init() {
    if (getXsdcConfig()->hasModules()) {
        for (const ap_xsd::Modules& xsdcModulesType : getXsdcConfig()->getModules()) {
            if (xsdcModulesType.has_module()) {
                for (const ap_xsd::Modules::Module& xsdcModule : xsdcModulesType.get_module()) {
                    if (xsdcModule.getName() != "r_submix") {
                        mModuleConfigurations->emplace_back(
                                xsdcModule.getName(),
                                VALUE_OR_FATAL(convertModuleConfigToAidl(xsdcModule)));
                    } else {
                        mModuleConfigurations->emplace_back(xsdcModule.getName(),
                                                            getStandardRemoteSubmixConfiguration());
                    }
                }
            }
        }
    }
}
}  // namespace aidl::android::hardware::audio::core::internal
