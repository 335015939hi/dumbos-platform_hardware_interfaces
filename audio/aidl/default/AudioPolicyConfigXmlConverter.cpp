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
#include <unistd.h>
#include <functional>
#include <unordered_map>

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <system/audio-base-utils.h>

#include "core-impl/AudioPolicyConfigXmlConverter.h"

using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioStreamType;

namespace xsd = ::android::audio::policy::configuration;

namespace aidl::android::hardware::audio::core::internal {

static const int DEFAULT_VOLUME_INDEX_MIN = 0;
static const int DEFAULT_VOLUME_INDEX_MAX = 100;
static const int VOLUME_INDEX_DEFERRED_TO_AUDIO_SERVICE = -1;

AudioHalVolumeCurve::CurvePoint AudioPolicyConfigXmlConverter::convertCurvePointToAidl(
        const std::string& xsdcCurvePoint) {
    AudioHalVolumeCurve::CurvePoint aidlCurvePoint;
    size_t pointDelimeterIdx = xsdcCurvePoint.find(",");
    aidlCurvePoint.index = std::stoi(xsdcCurvePoint.substr(0, pointDelimeterIdx));
    aidlCurvePoint.attenuationMb = std::stoi(xsdcCurvePoint.substr(pointDelimeterIdx + 1));
    return aidlCurvePoint;
}

AudioHalVolumeCurve AudioPolicyConfigXmlConverter::convertVolumeCurveToAidl(
        const xsd::Volume& xsdcVolumeCurve) {
    AudioHalVolumeCurve aidlVolumeCurve;
    aidlVolumeCurve.deviceCategory =
            static_cast<AudioHalVolumeCurve::DeviceCategory>(xsdcVolumeCurve.getDeviceCategory());
    if (xsdcVolumeCurve.hasRef()) {
        if (mVolumesReferenceMap.empty()) {
            mVolumesReferenceMap =
                    generateReferenceMap<xsd::Volumes, xsd::Reference>(mXsdcConfig->getVolumes());
        }
        aidlVolumeCurve.curvePoints =
                convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        mVolumesReferenceMap.at(xsdcVolumeCurve.getRef()).getPoint(),
                        std::bind(&AudioPolicyConfigXmlConverter::convertCurvePointToAidl, this,
                                  std::placeholders::_1));
    } else {
        aidlVolumeCurve.curvePoints =
                convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        xsdcVolumeCurve.getPoint(),
                        std::bind(&AudioPolicyConfigXmlConverter::convertCurvePointToAidl, this,
                                  std::placeholders::_1));
    }
    return aidlVolumeCurve;
}

void AudioPolicyConfigXmlConverter::mapStreamToVolumeCurve(const xsd::Volume& xsdcVolumeCurve) {
    mStreamToVolumeCurvesMap[xsdcVolumeCurve.getStream()].push_back(
            convertVolumeCurveToAidl(xsdcVolumeCurve));
}

AudioHalEngineConfig& AudioPolicyConfigXmlConverter::getAidlEngineConfig() {
    if (mAidlEngineConfig.volumeGroups.empty() && mXsdcConfig->hasVolumes()) {
        parseVolumes();
    }
    return mAidlEngineConfig;
}

void AudioPolicyConfigXmlConverter::mapStreamsToVolumeCurves() {
    if (mXsdcConfig->hasVolumes()) {
        for (const xsd::Volumes& xsdcWrapperType : mXsdcConfig->getVolumes()) {
            for (const xsd::Volume& xsdcVolume : xsdcWrapperType.getVolume()) {
                mapStreamToVolumeCurve(xsdcVolume);
            }
        }
    }
}

void AudioPolicyConfigXmlConverter::addVolumeGroupstoEngineConfig() {
    for (const auto& [xsdcStream, volumeCurves] : mStreamToVolumeCurvesMap) {
        AudioHalVolumeGroup volumeGroup;
        volumeGroup.name = xsd::toString(xsdcStream);
        if (static_cast<int>(xsdcStream) >= AUDIO_STREAM_PUBLIC_CNT) {
            volumeGroup.minIndex = DEFAULT_VOLUME_INDEX_MIN;
            volumeGroup.maxIndex = DEFAULT_VOLUME_INDEX_MAX;
        } else {
            volumeGroup.minIndex = VOLUME_INDEX_DEFERRED_TO_AUDIO_SERVICE;
            volumeGroup.maxIndex = VOLUME_INDEX_DEFERRED_TO_AUDIO_SERVICE;
        }
        volumeGroup.volumeCurves = volumeCurves;
        mAidlEngineConfig.volumeGroups.push_back(std::move(volumeGroup));
    }
}

void AudioPolicyConfigXmlConverter::parseVolumes() {
    if (mStreamToVolumeCurvesMap.empty() && mXsdcConfig->hasVolumes()) {
        mapStreamsToVolumeCurves();
        addVolumeGroupstoEngineConfig();
    }
}
}  // namespace aidl::android::hardware::audio::core::internal
