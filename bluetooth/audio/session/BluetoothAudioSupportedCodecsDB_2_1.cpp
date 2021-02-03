/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderSessionCodecsDB_2_1"

#include "BluetoothAudioSupportedCodecsDB_2_1.h"

#include <android-base/logging.h>

namespace android {
namespace bluetooth {
namespace audio {

using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;

static const ::android::hardware::bluetooth::audio::V2_1::PcmParameters
    kDefaultSoftwarePcmCapabilities_2_1 = {
        .sampleRate = static_cast<
            ::android::hardware::bluetooth::audio::V2_1::SampleRate>(
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_44100 |
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_48000 |
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_88200 |
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_96000 |
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_16000 |
            ::android::hardware::bluetooth::audio::V2_1::SampleRate::
                RATE_24000),
        .channelMode =
            static_cast<ChannelMode>(ChannelMode::MONO | ChannelMode::STEREO),
        .bitsPerSample = static_cast<BitsPerSample>(BitsPerSample::BITS_16 |
                                                    BitsPerSample::BITS_24 |
                                                    BitsPerSample::BITS_32)};

std::vector<::android::hardware::bluetooth::audio::V2_1::PcmParameters>
GetSoftwarePcmCapabilities_2_1() {
  return std::vector<
      ::android::hardware::bluetooth::audio::V2_1::PcmParameters>(
      1, kDefaultSoftwarePcmCapabilities_2_1);
}

std::vector<CodecCapabilities> GetOffloadCodecCapabilities(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&) {
  // TODO
  return std::vector<CodecCapabilities>(0);
}

bool IsSoftwarePcmConfigurationValid_2_1(
    const ::android::hardware::bluetooth::audio::V2_1::PcmParameters&
        pcm_config) {
  // TODO: sum of all 2.0 and 2.1 configurations
  if (pcm_config.sampleRate & kDefaultSoftwarePcmCapabilities_2_1.sampleRate &&
      pcm_config.bitsPerSample &
          kDefaultSoftwarePcmCapabilities_2_1.bitsPerSample &&
      pcm_config.channelMode &
          kDefaultSoftwarePcmCapabilities_2_1.channelMode &&
      pcm_config.dataIntervalUs != 0) {
    return true;
  }
  return true;
}

bool IsOffloadCodecConfigurationValid(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type,
    const ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration&
        codec_config) {
  if (session_type != ::android::hardware::bluetooth::audio::V2_1::SessionType::
                          A2DP_HARDWARE_OFFLOAD_DATAPATH) {
    LOG(ERROR) << __func__
               << ": Invalid SessionType=" << toString(session_type);
    return false;
  } else if (codec_config.encodedAudioBitrate < 0x00000001 ||
             0x00ffffff < codec_config.encodedAudioBitrate) {
    LOG(ERROR) << __func__ << ": Unsupported Codec Configuration="
               << toString(codec_config);
    return false;
  }
  // TODO: make proper checks!!
  return true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
