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

#define LOG_TAG "BTAudioProviderSessionCodecsDB"

#include "OffloadSupportedCodecsDB.h"

#include <android-base/logging.h>

namespace android {
namespace bluetooth {
namespace audio {

using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;

using PcmDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::PcmDataConfiguration;
using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

// Default Supported Codecs
// SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
static constexpr CodecCapability kDefaultOffloadSbcCodecCapability = {
    .codecType = CodecType::SBC,
    .pcmDataCapability = {.sampleRateBitMask = SampleRate::RATE_44100,
                          .bitsPerSampleBitMask = BitsPerSample::BITS_16,
                          .channelModeBitMask = static_cast<ChannelMode>(
                              ChannelMode::MONO | ChannelMode::STEREO)},
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

// AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
static constexpr CodecCapability kDefaultOffloadAacCodecCapability = {
    .codecType = CodecType::AAC,
    .pcmDataCapability = {.sampleRateBitMask = SampleRate::RATE_44100,
                          .bitsPerSampleBitMask = BitsPerSample::BITS_16,
                          .channelModeBitMask = ChannelMode::MONO},
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

// aptX: mSampleRate:(44100|48000), mBitsPerSample:(16), mChannelMode:(STEREO)
static constexpr CodecCapability kDefaultOffloadAptxCodecCapability = {
    .codecType = CodecType::APTX,
    .pcmDataCapability = {.sampleRateBitMask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000),
                          .bitsPerSampleBitMask = BitsPerSample::BITS_16,
                          .channelModeBitMask = ChannelMode::STEREO},
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

// aptX HD: mSampleRate:(44100|48000), mBitsPerSample:(24),
//          mChannelMode:(STEREO)
static constexpr CodecCapability kDefaultOffloadAptxHdCodecCapability = {
    .codecType = CodecType::APTX_HD,
    .pcmDataCapability = {.sampleRateBitMask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000),
                          .bitsPerSampleBitMask = BitsPerSample::BITS_24,
                          .channelModeBitMask = ChannelMode::STEREO},
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

// LDAC: mSampleRate:(44100|48000|88200|96000), mBitsPerSample:(16|24|32),
//       mChannelMode:(STEREO)
static constexpr CodecCapability kDefaultOffloadLdacCodecCapability = {
    .codecType = CodecType::LDAC,
    .pcmDataCapability = {.sampleRateBitMask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000 |
                              SampleRate::RATE_88200 | SampleRate::RATE_96000),
                          .bitsPerSampleBitMask = static_cast<BitsPerSample>(
                              BitsPerSample::BITS_16 | BitsPerSample::BITS_24 |
                              BitsPerSample::BITS_32),
                          .channelModeBitMask = ChannelMode::STEREO},
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

const std::vector<CodecCapability> kDefaultOffloadA2dpCodecCapabilities = {
    kDefaultOffloadSbcCodecCapability, kDefaultOffloadAacCodecCapability,
    kDefaultOffloadAptxCodecCapability, kDefaultOffloadAptxHdCodecCapability,
    kDefaultOffloadLdacCodecCapability};

static bool IsSbcEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  const bool is_scmst = encoded_config.isScmstEnabled;
  if (is_scmst) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else
    return true;
}

static bool IsAacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  const bool is_scmst = encoded_config.isScmstEnabled;
  if (is_scmst) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else
    return true;
}

static bool IsAptxEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  const bool is_scmst = encoded_config.isScmstEnabled;
  if (is_scmst) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else
    return true;
}

static bool IsAptxHdEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  const bool is_scmst = encoded_config.isScmstEnabled;
  if (is_scmst) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else
    return true;
}

static bool IsLdacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  const bool is_scmst = encoded_config.isScmstEnabled;
  if (is_scmst) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else
    return true;
}

bool IsOffloadCodecConfigurationValid(const CodecConfiguration& codec_config) {
  const CodecCapability::PcmDataCapability* default_offload_pcm_capability;
  switch (codec_config.encodedDataConfiguration.codecType) {
    case CodecType::SBC:
      if (!IsSbcEncodedDataConfigurationValid(codec_config)) {
        return false;
      }
      default_offload_pcm_capability =
          &kDefaultOffloadSbcCodecCapability.pcmDataCapability;
      break;
    case CodecType::AAC:
      if (!IsAacEncodedDataConfigurationValid(codec_config)) {
        return false;
      }
      default_offload_pcm_capability =
          &kDefaultOffloadAacCodecCapability.pcmDataCapability;
      break;
    case CodecType::APTX:
      if (!IsAptxEncodedDataConfigurationValid(codec_config)) {
        return false;
      }
      default_offload_pcm_capability =
          &kDefaultOffloadAptxCodecCapability.pcmDataCapability;
      break;
    case CodecType::APTX_HD:
      if (!IsAptxHdEncodedDataConfigurationValid(codec_config)) {
        return false;
      }
      default_offload_pcm_capability =
          &kDefaultOffloadAptxHdCodecCapability.pcmDataCapability;
      break;
    case CodecType::LDAC:
      if (!IsLdacEncodedDataConfigurationValid(codec_config)) {
        return false;
      }
      default_offload_pcm_capability =
          &kDefaultOffloadLdacCodecCapability.pcmDataCapability;
      break;
    case CodecType::UNKNOWN:
      return false;
  }
  const PcmDataConfiguration& pcm_config = codec_config.pcmDataConfiguration;
  if (!(pcm_config.sampleRate &
        default_offload_pcm_capability->sampleRateBitMask) ||
      !(pcm_config.bitsPerSample &
        default_offload_pcm_capability->bitsPerSampleBitMask) ||
      !(pcm_config.channelMode &
        default_offload_pcm_capability->channelModeBitMask)) {
    LOG(WARNING) << __func__
                 << ": Unsupported PCM Configuration=" << toString(pcm_config);
    return false;
  }
  return true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
