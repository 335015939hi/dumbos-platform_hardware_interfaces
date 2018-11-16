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
#include "BluetoothAudioSessionReport.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using ::android::bluetooth::audio::BluetoothAudioSessionReport;
using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;

using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

A2dpOffloadAudioProvider::A2dpOffloadAudioProvider()
    : BluetoothAudioProvider() {
  session_type_ = SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH;
}

bool A2dpOffloadAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_);
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

  return BluetoothAudioProvider::startSession(hostIf, codecConfig, _hidl_cb);
}

Return<void> A2dpOffloadAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
  BluetoothAudioSessionReport::OnSessionStarted(session_type_, stack_iface_,
                                                nullptr, codec_config_);
  _hidl_cb(BluetoothAudioStatus::SUCCESS, DataMQ::Descriptor());
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
