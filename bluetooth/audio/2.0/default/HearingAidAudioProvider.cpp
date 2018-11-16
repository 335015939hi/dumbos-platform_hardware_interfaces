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

#define LOG_TAG "BTAudioProviderHearingAid"

#include <log/log.h>

#include "HearingAidAudioProvider.h"

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

bool HearingAidAudioProvider::build(const SessionType& sessionType) {
  // HearingAidAudioProvider can only support types of Hearing Aids
  session_type_ = SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH;
  return (sessionType == SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH);
}

Return<void> HearingAidAudioProvider::startSession(
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
   * G.722: mSampleRate:(16000|24000), mBitsPerSample:(16),
   * mChannelMode:(MONO|STEREO)
   */
  switch (pcmConfig.sampleRate) {
    case SampleRate::RATE_16000:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case SampleRate::RATE_24000:
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
  switch (pcmConfig.bitsPerSample) {
    case BitsPerSample::BITS_16:
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
  switch (pcmConfig.channelMode) {
    case ChannelMode::MONO:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case ChannelMode::STEREO:
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

void HearingAidAudioProvider::updateStreamTracksMetadata(
    const struct source_metadata* source_metadata) {
  if (has_session_ && stack_iface_) {
    BluetoothAudioProvider::updateStreamTracksMetadata(source_metadata);
  } else {
    LOG(WARNING) << __func__ << " - provider has NO session";
  }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
