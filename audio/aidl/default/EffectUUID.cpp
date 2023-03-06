/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "include/effect-impl/EffectUUID.h"
#include <memory>
#include <ostream>
#define LOG_TAG "AHAL_EffectUUID"
#include <android-base/logging.h>
#include <android-base/no_destructor.h>
#include <pthread.h>
#include <sys/resource.h>

#include "effect-impl/EffectThread.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

using ::aidl::android::media::audio::common::AudioUuid;
using ::android::base::NoDestructor;

AudioUuid stringToUuid(const char* str) {
    AudioUuid uuid{};
    RETURN_VALUE_IF(!str, uuid, "nullPtr");

    uint32_t tmp[10];
    RETURN_VALUE_IF(
            (sscanf(str, "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x", tmp, tmp + 1, tmp + 2,
                    tmp + 3, tmp + 4, tmp + 5, tmp + 6, tmp + 7, tmp + 8, tmp + 9) < 10),
            uuid, "wrongSize");

    uuid.timeLow = (uint32_t)tmp[0];
    uuid.timeMid = (uint16_t)tmp[1];
    uuid.timeHiAndVersion = (uint16_t)tmp[2];
    uuid.clockSeq = (uint16_t)tmp[3];
    uuid.node.insert(uuid.node.end(), {(uint8_t)tmp[4], (uint8_t)tmp[5], (uint8_t)tmp[6],
                                       (uint8_t)tmp[7], (uint8_t)tmp[8], (uint8_t)tmp[9]});
    return uuid;
}

const AudioUuid& EffectNullUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEffectNullUuidStr));
    return *uuid;
}

const AudioUuid& EffectZeroUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEffectZeroUuidStr));
    return *uuid;
}

// AcousticEchoCanceler
const AudioUuid& AcousticEchoCancelerSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kAcousticEchoCancelerSwImplUuidStr));
    return *uuid;
}

const AudioUuid& AcousticEchoCancelerTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_AEC));
    return *uuid;
}

// AutomaticGainControlV1
const AudioUuid& AutomaticGainControlV1SwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kAutomaticGainControlV1SwImplUuidStr));
    return *uuid;
}

const AudioUuid& AutomaticGainControlV1TypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_AGC1));
    return *uuid;
}

// AutomaticGainControlV2
const AudioUuid& AutomaticGainControlV2SwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kAutomaticGainControlV2SwImplUuidStr));
    return *uuid;
}

const AudioUuid& AutomaticGainControlV2TypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_AGC2));
    return *uuid;
}

// BassBoost
const AudioUuid& BassBoostSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kBassBoostSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& BassBoostBundleImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kBassBoostBundleImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& BassBoostProxyUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kBassBoostProxyUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& BassBoostTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_BASS_BOOST));
    return *uuid;
}

// Downmix
const ::aidl::android::media::audio::common::AudioUuid& DownmixSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kDownmixSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& DownmixImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kDownmixImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& DownmixTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_DOWNMIX));
    return *uuid;
}

// DynamicsProcessing
const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kDynamicsProcessingSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kDynamicsProcessingImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_DYNAMICS_PROCESSING));
    return *uuid;
}

// Equalizer
const ::aidl::android::media::audio::common::AudioUuid& EqualizerSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEqualizerSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& EqualizerBundleImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEqualizerBundleImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& EqualizerProxyUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEqualizerProxyUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& EqualizerTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_EQUALIZER));
    return *uuid;
}

// HapticGenerator
const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kHapticGeneratorSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kHapticGeneratorImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_HAPTIC_GENERATOR));
    return *uuid;
}

// LoudnessEnhancer
const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kLoudnessEnhancerSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kLoudnessEnhancerImplUuidStr));
    return *uuid;
}
const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_LOUDNESS_ENHANCER));
    return *uuid;
}

// EnvReverb
const ::aidl::android::media::audio::common::AudioUuid& EnvReverbSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kEnvReverbSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& AuxEnvReverbImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kAuxEnvReverbImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& InsertEnvReverbImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kInsertEnvReverbImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& EnvReverbTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_ENV_REVERB));
    return *uuid;
}

// NoiseSuppression
const AudioUuid& NoiseSuppressionSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kNoiseSuppressionSwImplUuidStr));
    return *uuid;
}

const AudioUuid& NoiseSuppressionTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_NS));
    return *uuid;
}

// PresetReverb
const ::aidl::android::media::audio::common::AudioUuid& PresetReverbSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kPresetReverbSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& AuxPresetReverbImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kAuxPresetReverbImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& InsertPresetReverbImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kInsertPresetReverbImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& PresetReverbTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_PRESET_REVERB));
    return *uuid;
}

// Virtualizer
const ::aidl::android::media::audio::common::AudioUuid& VirtualizerSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVirtualizerSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VirtualizerBundleImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVirtualizerBundleImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VirtualizerProxyUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVirtualizerProxyUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VirtualizerTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_VIRTUALIZER));
    return *uuid;
}

// Visualizer
const ::aidl::android::media::audio::common::AudioUuid& VisualizerSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVisualizerSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VisualizerImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVisualizerImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VisualizerTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(
            stringToUuid(Descriptor::EFFECT_TYPE_UUID_VISUALIZER));
    return *uuid;
}

// Volume
const ::aidl::android::media::audio::common::AudioUuid& VolumeSwImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVolumeSwImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VolumeBundleImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kVolumeBundleImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& VolumeTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(Descriptor::EFFECT_TYPE_UUID_VOLUME));
    return *uuid;
}

// Extension
const ::aidl::android::media::audio::common::AudioUuid& ExtensionEffectImplUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid(kExtensionEffectImplUuidStr));
    return *uuid;
}

const ::aidl::android::media::audio::common::AudioUuid& ExtensionEffectTypeUUID() {
    static const NoDestructor<AudioUuid> uuid(stringToUuid("fa81dbde-588b-11ed-9b6a-0242ac120002"));
    return *uuid;
}

const std::map<const std::string, const std::string>& UuidNameTypeMap() {
    static const NoDestructor<std::map<const std::string, const std::string>> uuidMap(
            {{"acoustic_echo_canceler", Descriptor::EFFECT_TYPE_UUID_AEC},
             {"automatic_gain_control_v1", Descriptor::EFFECT_TYPE_UUID_AGC1},
             {"automatic_gain_control_v2", Descriptor::EFFECT_TYPE_UUID_AGC2},
             {"bassboost", Descriptor::EFFECT_TYPE_UUID_BASS_BOOST},
             {"downmix", Descriptor::EFFECT_TYPE_UUID_DOWNMIX},
             {"dynamics_processing", Descriptor::EFFECT_TYPE_UUID_DYNAMICS_PROCESSING},
             {"equalizer", Descriptor::EFFECT_TYPE_UUID_EQUALIZER},
             {"extensioneffect", "fa81dd00-588b-11ed-9b6a-0242ac120002"},
             {"haptic_generator", Descriptor::EFFECT_TYPE_UUID_HAPTIC_GENERATOR},
             {"loudness_enhancer", Descriptor::EFFECT_TYPE_UUID_LOUDNESS_ENHANCER},
             {"noise_suppression", Descriptor::EFFECT_TYPE_UUID_NS},
             {"env_reverb", Descriptor::EFFECT_TYPE_UUID_ENV_REVERB},
             {"reverb_env_aux", Descriptor::EFFECT_TYPE_UUID_ENV_REVERB},
             {"reverb_env_ins", Descriptor::EFFECT_TYPE_UUID_ENV_REVERB},
             {"preset_reverb", Descriptor::EFFECT_TYPE_UUID_PRESET_REVERB},
             {"reverb_pre_aux", Descriptor::EFFECT_TYPE_UUID_PRESET_REVERB},
             {"reverb_pre_ins", Descriptor::EFFECT_TYPE_UUID_PRESET_REVERB},
             {"virtualizer", Descriptor::EFFECT_TYPE_UUID_VIRTUALIZER},
             {"visualizer", Descriptor::EFFECT_TYPE_UUID_VISUALIZER},
             {"volume", Descriptor::EFFECT_TYPE_UUID_VOLUME}});
    return *uuidMap;
}

}  // namespace aidl::android::hardware::audio::effect
