/*
 * Copyright 2020 HIMSA II K/S - www.himsa.dk. Represented by EHIMA -
 * www.ehima.com
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

#define LOG_TAG "BTAudioProviderLeAudio"

#include <android-base/logging.h>

#include "BluetoothAudioSessionReport.h"
#include "BluetoothAudioSupportedCodecsDB.h"
#include "LeAudioAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_1 {
namespace implementation {

using ::android::bluetooth::audio::BluetoothAudioSessionReport;
using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;

static constexpr uint32_t kBufferCount = 1;  // single buffer

LeAudioAudioProvider::LeAudioAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {}

bool LeAudioAudioProvider::isValid(const SessionType_2_1& sessionType) {
  return (sessionType == session_type_ && mDataMQ && mDataMQ->isValid());
}

Return<void> LeAudioAudioProvider::startSession_2_1(
    const sp<V2_0::IBluetoothAudioPort>& hostIf,
    const AudioConfiguration_2_1& audioConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  if (audioConfig.getDiscriminator() !=
      AudioConfiguration_2_1::hidl_discriminator::pcmConfig) {
    LOG(WARNING) << __func__
                 << " - Invalid Audio Configuration=" << toString(audioConfig);
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  } else if (!android::bluetooth::audio::IsSoftwarePcmConfigurationValid(
                 audioConfig.pcmConfig())) {
    LOG(WARNING) << __func__ << " - Unsupported PCM Configuration="
                 << toString(audioConfig.pcmConfig());
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  uint32_t kDataMqSize = 0;
  switch (audioConfig.pcmConfig().sampleRate) {
  case SampleRate::RATE_16000:
    kDataMqSize = 16000;
    break;
  case SampleRate::RATE_24000:
    kDataMqSize = 24000;
    break;
  case SampleRate::RATE_44100:
    kDataMqSize = 44100;
    break;
  case SampleRate::RATE_48000:
    kDataMqSize = 48000;
    break;
  case SampleRate::RATE_88200:
    kDataMqSize = 88200;
    break;
  case SampleRate::RATE_96000:
    kDataMqSize = 96000;
    break;
  case SampleRate::RATE_176400:
    kDataMqSize = 176400;
    break;
  case SampleRate::RATE_192000:
    kDataMqSize = 192000;
    break;
  default:
    /* This should never happen it would be caught while validating
     * parameters.
     */
    break;
  }

  /* Number of samples per millisecond */
  kDataMqSize = ceil(kDataMqSize/1000);

  switch (audioConfig.pcmConfig().channelMode) {
  case ChannelMode::MONO:
    break;
  case ChannelMode::STEREO:
    kDataMqSize *= 2;
    break;
  default:
    /* This should never happen it would be caught while validating
     * parameters.
     */
    break;
  }

  switch (audioConfig.pcmConfig().bitsPerSample) {
  case BitsPerSample::BITS_16:
    kDataMqSize *= 2;
    break;
  case BitsPerSample::BITS_24:
    kDataMqSize *= 3;
    break;
  case BitsPerSample::BITS_32:
    kDataMqSize *= 4;
    break;
  default:
    /* This should never happen it would be caught while validating
     * parameters.
     */
    break;
  }

  kDataMqSize *= (audioConfig.pcmConfig().dataIntervalMs * kBufferCount);

  LOG(INFO) << __func__ << " - size of audio buffer " << kDataMqSize
            << " byte(s)";

  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
    session_type_ = SessionType_2_1::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "data MQ is invalid");
  }

  return BluetoothAudioProvider::startSession_2_1(hostIf, audioConfig, _hidl_cb);
}

Return<void> LeAudioAudioProvider::onSessionReady(startSession_cb _hidl_cb) {
  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport::OnSessionStarted(
        session_type_, stack_iface_, mDataMQ->getDesc(), audio_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
