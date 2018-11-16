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
#define LOG_TAG "BTAudioProviderStub"

#include "BluetoothAudioProvider.h"
#include "BluetoothAudioSessionReport.h"

#include <android-base/stringprintf.h>

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
#ifndef PCM_FRAME_SIZE
#define PCM_FRAME_SIZE 4  // 16 bits per sample / stereo
#endif                    /* PCM_FRAME_SIZE */
#ifndef PCM_FRAME_COUNT
#define PCM_FRAME_COUNT 128
#endif /* PCM_FRAME_COUNT */

#ifndef RTP_FRAME_SIZE
#define RTP_FRAME_SIZE (PCM_FRAME_SIZE * PCM_FRAME_COUNT)
#endif /* RTP_FRAME_SIZE */
#ifndef RTP_FRAME_COUNT
#define RTP_FRAME_COUNT 7 /* full of size within 1 tick (20ms) */
#endif                    /* RTP_FRAME_COUNT */

#define BUFFER_SIZE (RTP_FRAME_SIZE * RTP_FRAME_COUNT)
//#define DOUBLE_BUFFER 1 /* single buffer */
#define DOUBLE_BUFFER 2 /* double buffer */
//#define DOUBLE_BUFFER 3 /* triple buffer */

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
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::TimeSpec;

BluetoothAudioProvider::BluetoothAudioProvider()
    : session_type_(SessionType::UNKNOWN),
      has_session_(false),
      death_recipient_(new BluetoothAudioDeathRecipient(this)),
      unlink_cb_(nullptr) {
  LOG(INFO) << __func__ << " - size of audio data path "
            << (BUFFER_SIZE * DOUBLE_BUFFER) << " byte(s)";
  std::unique_ptr<DataMQ> tempDataMQ(new DataMQ(BUFFER_SIZE * DOUBLE_BUFFER,
                                                /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "data MQ is invalid");
    mDataMQ = nullptr;
  }
}

bool BluetoothAudioProvider::build(const SessionType& sessionType) {
  // BluetoothAudioProviderSub is well to support all types
  session_type_ = sessionType;
  return true;
}

Return<void> BluetoothAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  const PcmDataConfiguration pcmConfig = codecConfig.pcmDataConfiguration;
  if (pcmConfig.sampleRate == SampleRate::RATE_UNKNOWN ||
      pcmConfig.bitsPerSample == BitsPerSample::BITS_UNKNOWN ||
      pcmConfig.channelMode == ChannelMode::UNKNOWN) {
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  codec_config_ = codecConfig;
  stack_iface_ = hostIf;
  has_session_ = true;

  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", PcmConfig=[" << toString(codec_config_.pcmDataConfiguration)
            << "], EncodedConfig=[{.codecType = "
            << toString(codec_config_.encodedDataConfiguration.codecType)
            << ", .peerMtu = " << codec_config_.encodedDataConfiguration.peerMtu
            << "}]";

  death_recipient_->setHasDied(false);
  hostIf->linkToDeath(death_recipient_, 0);

  unlink_cb_ = [hostIf](sp<BluetoothAudioDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      LOG(INFO) << "Skipping unlink call, service died.";
    else
      hostIf->unlinkToDeath(death_recipient);
  };

  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport::OnSessionStarted(
        session_type_, hostIf, mDataMQ->getDesc(), codec_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    // A2DP_HARDWARE_OFFLOAD_DATAPATH
    BluetoothAudioSessionReport::OnSessionStarted(session_type_, hostIf,
                                                  nullptr, codec_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, DataMQ::Descriptor());
  }
  return Void();
}

Return<void> BluetoothAudioProvider::streamStarted(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  /**
   * Streaming on control path has started,
   * HAL server should start the streaming on data path.
   */
  if (has_session_) {
    BluetoothAudioSessionReport::ReportControlStatus(session_type_, true,
                                                     status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }

  return Void();
}

Return<void> BluetoothAudioProvider::streamSuspended(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  /**
   * Streaming on control path has suspend,
   * HAL server should suspend the streaming on data path.
   */
  if (has_session_) {
    BluetoothAudioSessionReport::ReportControlStatus(session_type_, false,
                                                     status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }

  return Void();
}

Return<void> BluetoothAudioProvider::endSession() {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);

  if (unlink_cb_) {
    unlink_cb_(death_recipient_);
    death_recipient_->setHasDied(true);
    unlink_cb_ = nullptr;
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " ended twice";
  }

  if (has_session_) {
    BluetoothAudioSessionReport::OnSessionEnded(session_type_);
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
  }

  /**
   * Cleanup the audio platform as remote audio device is no
   * longer active
   */
  has_session_ = false;
  stack_iface_ = nullptr;

  codec_config_.pcmDataConfiguration = ::android::bluetooth::audio::
      BluetoothAudioSession::kInvalidPcmDataConfiguration;

  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
