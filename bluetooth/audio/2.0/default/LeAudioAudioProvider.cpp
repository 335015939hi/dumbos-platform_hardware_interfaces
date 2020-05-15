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
namespace V2_0 {
namespace implementation {

using ::android::bluetooth::audio::BluetoothAudioSessionReport;
using ::android::hardware::Void;

/* TODO re-check parameters for le audio. For now it's ok to test this static
 * configuration
 */
static constexpr uint32_t kOutPcmFrameSize = 4;  // 16 bits per sample / stereo
static constexpr uint32_t kOutPcmFrameCount = 128;
static constexpr uint32_t kOutRtpFrameSize =
    kOutPcmFrameSize * kOutPcmFrameCount;
static constexpr uint32_t kOutRtpFrameCount = 7;  // max counts by 1 tick (20ms)
static constexpr uint32_t kOutBufferSize = kOutRtpFrameSize * kOutRtpFrameCount;
static constexpr uint32_t kOutBufferCount = 1;  // single buffer
static constexpr uint32_t kOutDataMqSize = kOutBufferSize * kOutBufferCount;

static constexpr uint32_t kInPcmFrameSize = 2;  // 16 bits per sample / mono
static constexpr uint32_t kInPcmFrameCount = 128;
static constexpr uint32_t kInRtpFrameSize = kInPcmFrameSize * kInPcmFrameCount;
static constexpr uint32_t kInRtpFrameCount = 7;  // max counts by 1 tick (20ms)
static constexpr uint32_t kInBufferSize = kInRtpFrameSize * kInRtpFrameCount;
static constexpr uint32_t kInBufferCount = 1;  // single buffer
static constexpr uint32_t kInDataMqSize = kInBufferSize * kInBufferCount;

/* Output */
LeAudioOutputAudioProvider::LeAudioOutputAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {
  LOG(INFO) << __func__ << " - size of output audio buffers " << kOutDataMqSize
            << " byte(s)";
  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kOutDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
    session_type_ = SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate output data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "output data MQ is invalid");
  }
}

bool LeAudioOutputAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_ && mDataMQ && mDataMQ->isValid());
}

Return<void> LeAudioOutputAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  if (audioConfig.getDiscriminator() !=
      AudioConfiguration::hidl_discriminator::pcmConfig) {
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

  return BluetoothAudioProvider::startSession(hostIf, audioConfig, _hidl_cb);
}

Return<void> LeAudioOutputAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport::OnSessionStarted(
        session_type_, stack_iface_, mDataMQ->getDesc(), audio_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
  }
  return Void();
}

/* Input */
LeAudioInputAudioProvider::LeAudioInputAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {
  LOG(INFO) << __func__ << " - size of input audio buffers " << kInDataMqSize
            << " byte(s)";
  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kInDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
    session_type_ = SessionType::LE_AUDIO_SOFTWARE_DECODED_DATAPATH;
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate input data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "input data MQ is invalid");
  }
}

bool LeAudioInputAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_ && mDataMQ && mDataMQ->isValid());
}

Return<void> LeAudioInputAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  if (audioConfig.getDiscriminator() !=
      AudioConfiguration::hidl_discriminator::pcmConfig) {
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

  return BluetoothAudioProvider::startSession(hostIf, audioConfig, _hidl_cb);
}

Return<void> LeAudioInputAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
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
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
