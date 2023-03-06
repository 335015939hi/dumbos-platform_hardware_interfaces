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

#pragma once
#include <map>

#include <aidl/android/hardware/audio/effect/Descriptor.h>
#include <aidl/android/media/audio/common/AudioUuid.h>

#include "EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

using ::aidl::android::media::audio::common::AudioUuid;

::aidl::android::media::audio::common::AudioUuid stringToUuid(const char* str);

// Effect implementation UUID
constexpr char kEffectNullUuidStr[] = "ec7178ec-e5e1-4432-a3f4-4657e6795210";
const ::aidl::android::media::audio::common::AudioUuid& EffectNullUUID();

constexpr char kEffectZeroUuidStr[] = "00000000-0000-0000-0000-000000000000";
const ::aidl::android::media::audio::common::AudioUuid& EffectZeroUUID();

constexpr char kAcousticEchoCancelerSwImplUuidStr[] = "bb392ec0-8d4d-11e0-a896-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& AcousticEchoCancelerSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& AcousticEchoCancelerTypeUUID();

constexpr char kAutomaticGainControlV1SwImplUuidStr[] = "aa8130e0-66fc-11e0-bad0-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& AutomaticGainControlV1SwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& AutomaticGainControlV1TypeUUID();

constexpr char kAutomaticGainControlV2SwImplUuidStr[] = "89f38e65-d4d2-4d64-ad0e-2b3e799ea886";
const ::aidl::android::media::audio::common::AudioUuid& AutomaticGainControlV2SwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& AutomaticGainControlV2TypeUUID();

constexpr char kBassBoostSwImplUuidStr[] = "fa8181f2-588b-11ed-9b6a-0242ac120002";
constexpr char kBassBoostBundleImplUuidStr[] = "8631f300-72e2-11df-b57e-0002a5d5c51b";
constexpr char kBassBoostProxyUuidStr[] = "14804144-a5ee-4d24-aa88-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& BassBoostSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& BassBoostBundleImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& BassBoostProxyUUID();
const ::aidl::android::media::audio::common::AudioUuid& BassBoostTypeUUID();

constexpr char kDownmixSwImplUuidStr[] = "fa8187ba-588b-11ed-9b6a-0242ac120002";
constexpr char kDownmixImplUuidStr[] = "93f04452-e4fe-41cc-91f9-e475b6d1d69f";
const ::aidl::android::media::audio::common::AudioUuid& DownmixSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& DownmixImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& DownmixTypeUUID();

constexpr char kDynamicsProcessingSwImplUuidStr[] = "fa818d78-588b-11ed-9b6a-0242ac120002";
constexpr char kDynamicsProcessingImplUuidStr[] = "e0e6539b-1781-7261-676f-6d7573696340";
const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& DynamicsProcessingTypeUUID();

constexpr char kEqualizerSwImplUuidStr[] = "0bed4300-847d-11df-bb17-0002a5d5c51b";
constexpr char kEqualizerBundleImplUuidStr[] = "ce772f20-847d-11df-bb17-0002a5d5c51b";
constexpr char kEqualizerProxyUuidStr[] = "c8e70ecd-48ca-456e-8a4f-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& EqualizerSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& EqualizerBundleImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& EqualizerProxyUUID();
const ::aidl::android::media::audio::common::AudioUuid& EqualizerTypeUUID();

constexpr char kHapticGeneratorSwImplUuidStr[] = "fa819110-588b-11ed-9b6a-0242ac120002";
constexpr char kHapticGeneratorImplUuidStr[] = "97c4acd1-8b82-4f2f-832e-c2fe5d7a9931";
const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& HapticGeneratorTypeUUID();

constexpr char kLoudnessEnhancerSwImplUuidStr[] = "fa819610-588b-11ed-9b6a-0242ac120002";
constexpr char kLoudnessEnhancerImplUuidStr[] = "fa415329-2034-4bea-b5dc-5b381c8d1e2c";
const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& LoudnessEnhancerTypeUUID();

constexpr char kEnvReverbSwImplUuidStr[] = "fa819886-588b-11ed-9b6a-0242ac120002";
constexpr char kAuxEnvReverbImplUuidStr[] = "4a387fc0-8ab3-11df-8bad-0002a5d5c51b";
constexpr char kInsertEnvReverbImplUuidStr[] = "c7a511a0-a3bb-11df-860e-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& EnvReverbSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& AuxEnvReverbImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& InsertEnvReverbImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& EnvReverbTypeUUID();

constexpr char kNoiseSuppressionSwImplUuidStr[] = "c06c8400-8e06-11e0-9cb6-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& NoiseSuppressionSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& NoiseSuppressionTypeUUID();

constexpr char kPresetReverbSwImplUuidStr[] = "fa8199c6-588b-11ed-9b6a-0242ac120002";
constexpr char kAuxPresetReverbImplUuidStr[] = "f29a1400-a3bb-11df-8ddc-0002a5d5c51b";
constexpr char kInsertPresetReverbImplUuidStr[] = "172cdf00-a3bc-11df-a72f-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& PresetReverbSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& AuxPresetReverbImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& InsertPresetReverbImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& PresetReverbTypeUUID();

constexpr char kVirtualizerSwImplUuidStr[] = "fa819d86-588b-11ed-9b6a-0242ac120002";
constexpr char kVirtualizerBundleImplUuidStr[] = "1d4033c0-8557-11df-9f2d-0002a5d5c51b";
constexpr char kVirtualizerProxyUuidStr[] = "d3467faa-acc7-4d34-acaf-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& VirtualizerSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VirtualizerBundleImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VirtualizerProxyUUID();
const ::aidl::android::media::audio::common::AudioUuid& VirtualizerTypeUUID();

constexpr char kVisualizerSwImplUuidStr[] = "fa81a0f6-588b-11ed-9b6a-0242ac120002";
constexpr char kVisualizerImplUuidStr[] = "d069d9e0-8329-11df-9168-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& VisualizerSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VisualizerImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VisualizerTypeUUID();

constexpr char kVolumeSwImplUuidStr[] = "fa81a718-588b-11ed-9b6a-0242ac120002";
constexpr char kVolumeBundleImplUuidStr[] = "119341a0-8469-11df-81f9-0002a5d5c51b";
const ::aidl::android::media::audio::common::AudioUuid& VolumeSwImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VolumeBundleImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& VolumeTypeUUID();

constexpr char kExtensionEffectImplUuidStr[] = "fa81dd00-588b-11ed-9b6a-0242ac120002";
const ::aidl::android::media::audio::common::AudioUuid& ExtensionEffectImplUUID();
const ::aidl::android::media::audio::common::AudioUuid& ExtensionEffectTypeUUID();

/**
 * @brief A map between effect name and effect type UUID string.
 * All <name> attribution in effect/effectProxy of audio_effects.xml should be listed in this map.
 * We need this map is because existing audio_effects.xml don't have a type UUID defined.
 *
 * As an extension example, the type UUID is not defined in Descriptor.aidl, instead the UUID can be
 * listed in this map, see item: {"extensioneffect", "fa81dd00-588b-11ed-9b6a-0242ac120002"}.
 */
const std::map<const std::string, const std::string>& UuidNameTypeMap();

}  // namespace aidl::android::hardware::audio::effect
