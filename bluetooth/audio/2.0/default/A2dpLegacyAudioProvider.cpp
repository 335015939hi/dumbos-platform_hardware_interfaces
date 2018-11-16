/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderA2dpLegacy"

#include <log/log.h>

#include "A2dpLegacyAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::hidl_death_recipient;
using android::hardware::Void;

using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;

using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

bool A2dpLegacyAudioProvider::build(const SessionType& sessionType) {
  // A2dpLegacyAudioProvider can only support types of A2DP
  session_type_ = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
  return (sessionType == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
}

Return<void> A2dpLegacyAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  const PcmDataConfiguration pcmConfig = codecConfig.pcmDataConfiguration;
  /**
   * SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
   * AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
   * aptX: mSampleRate:(44100|48000), mBitsPerSample:(16), mChannelMode:(STEREO)
   * aptX HD: mSampleRate:(44100|48000), mBitsPerSample:(24),
   *          mChannelMode:(STEREO)
   * LDAC: mSampleRate:(44100|48000|88200|96000), mBitsPerSample:(16|24|32),
   *       mChannelMode:(STEREO)
   */
  switch (pcmConfig.sampleRate) {
    case SampleRate::RATE_44100:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case SampleRate::RATE_48000:
      // assume those rates are supported by all codecs
      break;
    case SampleRate::RATE_88200:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case SampleRate::RATE_96000:
      // this rates are supported by LDAC only
      if (codecConfig.encodedDataConfiguration.codecType == CodecType::LDAC)
        break;
#ifdef FALLTHROUGH_INTENDED
      else
        FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    default:
      LOG(WARNING) << __func__
                   << " - Unsupported PCM Configuration=" << toString(pcmConfig)
                   << ", CodecType="
                   << toString(codecConfig.encodedDataConfiguration.codecType);
      _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
               DataMQ::Descriptor());
      return Void();
  }
  switch (pcmConfig.bitsPerSample) {
    case BitsPerSample::BITS_16:
      // assume those formats are supported by all codecs
      break;
    case BitsPerSample::BITS_24:
      // those formats are supported by aptX HD and LDAC only
      if (codecConfig.encodedDataConfiguration.codecType ==
              CodecType::APTX_HD ||
          codecConfig.encodedDataConfiguration.codecType == CodecType::LDAC)
        break;
#ifdef FALLTHROUGH_INTENDED
      else
        FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case BitsPerSample::BITS_32:
      // this format is supported by LDAC only
      if (codecConfig.encodedDataConfiguration.codecType == CodecType::LDAC)
        break;
#ifdef FALLTHROUGH_INTENDED
      else
        FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    default:
      LOG(WARNING) << __func__
                   << " - Unsupported PCM Configuration=" << toString(pcmConfig)
                   << ", CodecType="
                   << toString(codecConfig.encodedDataConfiguration.codecType);
      _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
               DataMQ::Descriptor());
      return Void();
  }
  switch (pcmConfig.channelMode) {
    case ChannelMode::MONO:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case ChannelMode::STEREO:
      // assume those channels are supported by all codecs
      break;
    default:
      LOG(WARNING) << __func__
                   << " - Unsupported PCM Configuration=" << toString(pcmConfig)
                   << ", CodecType="
                   << toString(codecConfig.encodedDataConfiguration.codecType);
      _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
               DataMQ::Descriptor());
      return Void();
  }

  return BluetoothAudioProvider::startSession(hostIf, codecConfig, _hidl_cb);
}

void A2dpLegacyAudioProvider::updateStreamTracksMetadata(
    const struct source_metadata* source_metadata) {
  if (has_session_ && stack_iface_) {
    LOG(DEBUG) << __func__ << " - " << source_metadata->track_count
               << " track(s) is no needed for SessionType="
               << toString(session_type_);
  } else {
    LOG(INFO) << __func__ << " - provider has NO session";
  }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
