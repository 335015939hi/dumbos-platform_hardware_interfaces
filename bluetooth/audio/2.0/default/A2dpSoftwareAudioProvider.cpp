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

#define LOG_TAG "BTAudioProviderA2dpSoftware"

#include <log/log.h>

#include "A2dpSoftwareAudioProvider.h"
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

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
static constexpr uint32_t kPcmFrameSize = 4;  // 16 bits per sample / stereo
static constexpr uint32_t kPcmFrameCount = 128;
static constexpr uint32_t kRtpFrameSize = kPcmFrameSize * kPcmFrameCount;
static constexpr uint32_t kRtpFrameCount = 7;  // max counts by 1 tick (20ms)
static constexpr uint32_t kBufferSize = kRtpFrameSize * kRtpFrameCount;
static constexpr uint32_t kBufferCount = 2;  // double buffer
static constexpr uint32_t kDataMqSize = kBufferSize * kBufferCount;

A2dpSoftwareAudioProvider::A2dpSoftwareAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {
  LOG(INFO) << __func__ << " - size of audio buffer " << kDataMqSize
            << " byte(s)";
  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
    session_type_ = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "data MQ is invalid");
  }
}

bool A2dpSoftwareAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_ && mDataMQ && mDataMQ->isValid());
}

Return<void> A2dpSoftwareAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  const PcmDataConfiguration& pcmConfig = codecConfig.pcmDataConfiguration;
  /**
   * SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
   * AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
   * aptX: mSampleRate:(44100|48000), mBitsPerSample:(16), mChannelMode:(STEREO)
   * aptX HD: mSampleRate:(44100|48000), mBitsPerSample:(24),
   *          mChannelMode:(STEREO)
   * LDAC: mSampleRate:(44100|48000|88200|96000), mBitsPerSample:(16|24|32),
   *       mChannelMode:(STEREO)
   */
  if (pcmConfig.sampleRate != SampleRate::RATE_44100 &&
      pcmConfig.sampleRate != SampleRate::RATE_48000 &&
      pcmConfig.sampleRate != SampleRate::RATE_88200 &&
      pcmConfig.sampleRate != SampleRate::RATE_96000 &&
      pcmConfig.bitsPerSample != BitsPerSample::BITS_16 &&
      pcmConfig.bitsPerSample != BitsPerSample::BITS_24 &&
      pcmConfig.bitsPerSample != BitsPerSample::BITS_32 &&
      pcmConfig.channelMode != ChannelMode::MONO &&
      pcmConfig.channelMode != ChannelMode::STEREO) {
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

Return<void> A2dpSoftwareAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport::OnSessionStarted(
        session_type_, stack_iface_, mDataMQ->getDesc(), codec_config_);
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
