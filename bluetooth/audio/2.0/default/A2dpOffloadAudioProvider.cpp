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

#define LOG_TAG "BTAudioProviderA2dpOffload"

#include <log/log.h>

#include "A2dpOffloadAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;

using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

A2dpOffloadAudioProvider::A2dpOffloadAudioProvider() {
  session_type_ = SessionType::UNKNOWN;
  has_session_ = false;
  death_recipient_ = new BluetoothAudioDeathRecipient(this);
  unlink_cb_ = nullptr;
  // A2DP hardware offload has no software datapath
  mDataMQ = nullptr;
}

bool A2dpOffloadAudioProvider::build(const SessionType& sessionType) {
  // A2dpOffloadAudioProvider can only support types of A2DP
  session_type_ = SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH;
  return (sessionType == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH);
}

Return<void> A2dpOffloadAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  switch (codecConfig.encodedDataConfiguration.codecType) {
    case CodecType::SBC:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case CodecType::AAC:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case CodecType::APTX:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case CodecType::APTX_HD:
#ifdef FALLTHROUGH_INTENDED
      FALLTHROUGH_INTENDED;
#endif /* FALLTHROUGH_INTENDED */
    case CodecType::LDAC:
      // assume A2DP hardware offloading supports those codecs
      break;
    default:
      LOG(WARNING) << __func__ << " - Unsupported CodecType="
                   << toString(codecConfig.encodedDataConfiguration.codecType)
                   << ", PCM Configuration="
                   << toString(codecConfig.pcmDataConfiguration);
      _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
               DataMQ::Descriptor());
      return Void();
  }

  // TODO: check CodecSpecific of EncodedDataConfiguration

  return A2dpLegacyAudioProvider::startSession(hostIf, codecConfig, _hidl_cb);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
