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

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <system/audio-base-utils.h>
#include <utils/Log.h>

#include "core-impl/AudioPolicyConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioStreamType;

namespace ap_xsd = android::audio::policy::configuration;

namespace aidl::android::hardware::audio::core::internal {

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

std::vector<std::pair<std::string, Module::Configuration>>&
AudioPolicyConfigXmlConverter::getModuleConfigs() {
    return mModuleConfigurations;
}

const AudioHalEngineConfig& AudioPolicyConfigXmlConverter::getAidlEngineConfig() {
    if (mAidlEngineConfig.volumeGroups.empty() && getXsdcConfig() &&
        getXsdcConfig()->hasVolumes()) {
        parseVolumes();
    }
    return mAidlEngineConfig;
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
                    mModuleConfigurations.emplace_back(
                            xsdcModule.getName(),
                            VALUE_OR_FATAL(convertModuleConfigToAidl(xsdcModule)));
                }
            }
        }
    }
}
}  // namespace aidl::android::hardware::audio::core::internal
