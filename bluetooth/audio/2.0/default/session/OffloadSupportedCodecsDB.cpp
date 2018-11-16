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

using ::android::hardware::bluetooth::audio::V2_0::AacObjectType;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::LdacChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;

using PcmDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::PcmDataConfiguration;
using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

// Default Supported Codecs
// SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
static const CodecCapability kDefaultOffloadSbcCodecCapability = {
    .codecType = CodecType::SBC,
    .pcmDataCapability = {.sampleRateBitmask = SampleRate::RATE_44100,
                          .bitsPerSampleBitmask = BitsPerSample::BITS_16,
                          .channelModeBitmask = static_cast<ChannelMode>(
                              ChannelMode::MONO | ChannelMode::STEREO)},
    .isScmstSupported = false,
    .codecSpecific = {}};
static const CodecCapability::CodecSpecific::SbcDataCapability
    kDefaultSbcDataCapability = {
        .channelModeBitmask = static_cast<SbcChannelMode>(
            SbcChannelMode::MONO | SbcChannelMode::JOINT_STEREO),
        // all blocks | subbands 8 | Loudness
        .codecParametersBitmask = (0xf0 | 0x04 | 0x01),
        .minBitpool = 2,
        .maxBitpool = 53};

// AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
static const CodecCapability kDefaultOffloadAacCodecCapability = {
    .codecType = CodecType::AAC,
    .pcmDataCapability = {.sampleRateBitmask = SampleRate::RATE_44100,
                          .bitsPerSampleBitmask = BitsPerSample::BITS_16,
                          .channelModeBitmask = ChannelMode::MONO},
    .isScmstSupported = false,
    .codecSpecific = {}};
static const CodecCapability::CodecSpecific::AacDataCapability
    kDefaultAacDataCapability = {
        .aacObjectTypeBitmask = AacObjectType::MPEG2_LC,
        .isVariableBitRateSupported = false};

// aptX: mSampleRate:(44100|48000), mBitsPerSample:(16), mChannelMode:(STEREO)
static const CodecCapability kDefaultOffloadAptxCodecCapability = {
    .codecType = CodecType::APTX,
    .pcmDataCapability = {.sampleRateBitmask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000),
                          .bitsPerSampleBitmask = BitsPerSample::BITS_16,
                          .channelModeBitmask = ChannelMode::STEREO},
    .isScmstSupported = false,
    // no codec specific for aptX
    .codecSpecific = {}};

// aptX HD: mSampleRate:(44100|48000), mBitsPerSample:(24),
//          mChannelMode:(STEREO)
static const CodecCapability kDefaultOffloadAptxHdCodecCapability = {
    .codecType = CodecType::APTX_HD,
    .pcmDataCapability = {.sampleRateBitmask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000),
                          .bitsPerSampleBitmask = BitsPerSample::BITS_24,
                          .channelModeBitmask = ChannelMode::STEREO},
    .isScmstSupported = false,
    // no codec specific for aptX HD
    .codecSpecific = {}};

// LDAC: mSampleRate:(44100|48000|88200|96000), mBitsPerSample:(16|24|32),
//       mChannelMode:(STEREO)
static const CodecCapability kDefaultOffloadLdacCodecCapability = {
    .codecType = CodecType::LDAC,
    .pcmDataCapability = {.sampleRateBitmask = static_cast<SampleRate>(
                              SampleRate::RATE_44100 | SampleRate::RATE_48000 |
                              SampleRate::RATE_88200 | SampleRate::RATE_96000),
                          .bitsPerSampleBitmask = static_cast<BitsPerSample>(
                              BitsPerSample::BITS_16 | BitsPerSample::BITS_24 |
                              BitsPerSample::BITS_32),
                          .channelModeBitmask = ChannelMode::STEREO},
    .isScmstSupported = false,
    .codecSpecific = {}};
static const CodecCapability::CodecSpecific::LdacDataCapability
    kDefaultLdacDataCapability = {
        .channelModeBitmask = static_cast<LdacChannelMode>(
            LdacChannelMode::DUAL | LdacChannelMode::STEREO)};

const std::vector<CodecCapability> kDefaultOffloadA2dpCodecCapabilities = {
    kDefaultOffloadSbcCodecCapability, kDefaultOffloadAacCodecCapability,
    kDefaultOffloadAptxCodecCapability, kDefaultOffloadAptxHdCodecCapability,
    kDefaultOffloadLdacCodecCapability};

static bool IsSbcEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config);
static bool IsAacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config);
static bool IsAptxEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config);
static bool IsAptxHdEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config);
static bool IsLdacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config);

static bool IsSbcEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  if ((encoded_config.isScmstEnabled &&
       !kDefaultOffloadSbcCodecCapability.isScmstSupported) ||
      (encoded_config.encodedAudioBitrate < 0x00000001 ||
       0x00ffffff < encoded_config.encodedAudioBitrate)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  const EncodedDataConfiguration::CodecSpecific::SbcData sbc_data =
      encoded_config.codecSpecific.sbcData();
  if ((sbc_data.channelMode != SbcChannelMode::JOINT_STEREO &&
       sbc_data.channelMode != SbcChannelMode::STEREO &&
       sbc_data.channelMode != SbcChannelMode::DUAL &&
       sbc_data.channelMode != SbcChannelMode::MONO) ||
      // has multiple Blocks
      ((sbc_data.codecParameters & 0xf0) != 0x10 &&
       (sbc_data.codecParameters & 0xf0) != 0x20 &&
       (sbc_data.codecParameters & 0xf0) != 0x40 &&
       (sbc_data.codecParameters & 0xf0) != 0x80) ||
      // has multiple Subbands or multiple allocation methods
      ((sbc_data.codecParameters & 0x0c) == 0x0c) ||
      ((sbc_data.codecParameters & 0x03) == 0x03) ||
      sbc_data.minBitpool > sbc_data.maxBitpool) {
    LOG(WARNING) << __func__ << ": Invalid EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else if (!(sbc_data.channelMode &
               kDefaultSbcDataCapability.channelModeBitmask) ||
             !((sbc_data.codecParameters &
                kDefaultSbcDataCapability.codecParametersBitmask) &
               0xf0) ||
             !((sbc_data.codecParameters &
                kDefaultSbcDataCapability.codecParametersBitmask) &
               0x0c) ||
             !((sbc_data.codecParameters &
                kDefaultSbcDataCapability.codecParametersBitmask) &
               0x03) ||
             (sbc_data.minBitpool < kDefaultSbcDataCapability.minBitpool ||
              kDefaultSbcDataCapability.maxBitpool < sbc_data.maxBitpool)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  return true;
}

static bool IsAacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  if ((encoded_config.isScmstEnabled &&
       !kDefaultOffloadAacCodecCapability.isScmstSupported) ||
      (encoded_config.encodedAudioBitrate < 0x00000001 ||
       0x00ffffff < encoded_config.encodedAudioBitrate)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  const EncodedDataConfiguration::CodecSpecific::AacData aac_data =
      encoded_config.codecSpecific.aacData();
  if (aac_data.aacObjectType != AacObjectType::MPEG2_LC &&
      aac_data.aacObjectType != AacObjectType::MPEG4_LC &&
      aac_data.aacObjectType != AacObjectType::MPEG4_LTP &&
      aac_data.aacObjectType != AacObjectType::MPEG4_SCALABLE) {
    LOG(WARNING) << __func__ << ": Invalid EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else if (!(aac_data.aacObjectType &
               kDefaultAacDataCapability.aacObjectTypeBitmask) ||
             (aac_data.variableBitRateEnabled &&
              !kDefaultAacDataCapability.isVariableBitRateSupported)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  return true;
}

static bool IsAptxEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  if ((encoded_config.isScmstEnabled &&
       !kDefaultOffloadAptxCodecCapability.isScmstSupported) ||
      (encoded_config.encodedAudioBitrate < 0x00000001 ||
       0x00ffffff < encoded_config.encodedAudioBitrate)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  return true;
}

static bool IsAptxHdEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  if ((encoded_config.isScmstEnabled &&
       !kDefaultOffloadAptxHdCodecCapability.isScmstSupported) ||
      (encoded_config.encodedAudioBitrate < 0x00000001 ||
       0x00ffffff < encoded_config.encodedAudioBitrate)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  return true;
}

static bool IsLdacEncodedDataConfigurationValid(
    const CodecConfiguration& codec_config) {
  const EncodedDataConfiguration& encoded_config =
      codec_config.encodedDataConfiguration;
  if ((encoded_config.isScmstEnabled &&
       !kDefaultOffloadLdacCodecCapability.isScmstSupported) ||
      (encoded_config.encodedAudioBitrate < 0x00000001 ||
       0x00ffffff < encoded_config.encodedAudioBitrate)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  const EncodedDataConfiguration::CodecSpecific::LdacData ldac_data =
      encoded_config.codecSpecific.ldacData();
  if (ldac_data.channelMode != LdacChannelMode::STEREO &&
      ldac_data.channelMode != LdacChannelMode::DUAL &&
      ldac_data.channelMode != LdacChannelMode::MONO) {
    LOG(WARNING) << __func__ << ": Invalid EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  } else if (!(ldac_data.channelMode &
               kDefaultLdacDataCapability.channelModeBitmask)) {
    LOG(WARNING) << __func__ << ": Unsupported EncodedDataConfiguration="
                 << toString(encoded_config);
    return false;
  }
  return true;
}

const std::vector<CodecCapability> GetOffloadCodecCapabilities(
    const SessionType& session_type) {
  if (session_type != SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
    return std::vector<CodecCapability>(0);
  }
  std::vector<CodecCapability> offload_a2dp_codec_caps =
      kDefaultOffloadA2dpCodecCapabilities;
  for (auto codec_caps : offload_a2dp_codec_caps) {
    switch (codec_caps.codecType) {
      case CodecType::SBC:
        codec_caps.codecSpecific.sbcDataCapability(kDefaultSbcDataCapability);
        continue;
      case CodecType::AAC:
        codec_caps.codecSpecific.aacDataCapability(kDefaultAacDataCapability);
        continue;
      case CodecType::LDAC:
        codec_caps.codecSpecific.ldacDataCapability(kDefaultLdacDataCapability);
        continue;
      case CodecType::APTX:
        FALLTHROUGH_INTENDED;
      case CodecType::APTX_HD:
        FALLTHROUGH_INTENDED;
      case CodecType::UNKNOWN:
        continue;
    }
  }
  return offload_a2dp_codec_caps;
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
  if ((pcm_config.sampleRate != SampleRate::RATE_44100 &&
       pcm_config.sampleRate != SampleRate::RATE_48000 &&
       pcm_config.sampleRate != SampleRate::RATE_88200 &&
       pcm_config.sampleRate != SampleRate::RATE_96000 &&
       pcm_config.sampleRate != SampleRate::RATE_176400 &&
       pcm_config.sampleRate != SampleRate::RATE_192000) ||
      (pcm_config.bitsPerSample != BitsPerSample::BITS_16 &&
       pcm_config.bitsPerSample != BitsPerSample::BITS_24 &&
       pcm_config.bitsPerSample != BitsPerSample::BITS_32) ||
      (pcm_config.channelMode != ChannelMode::MONO &&
       pcm_config.channelMode != ChannelMode::STEREO)) {
    LOG(WARNING) << __func__
                 << ": Invalid PCM Configuration=" << toString(pcm_config);
    return false;
  } else if (!(pcm_config.sampleRate &
               default_offload_pcm_capability->sampleRateBitmask) ||
             !(pcm_config.bitsPerSample &
               default_offload_pcm_capability->bitsPerSampleBitmask) ||
             !(pcm_config.channelMode &
               default_offload_pcm_capability->channelModeBitmask)) {
    LOG(WARNING) << __func__
                 << ": Unsupported PCM Configuration=" << toString(pcm_config);
    return false;
  }
  return true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
