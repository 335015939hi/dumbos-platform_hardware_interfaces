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

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

BluetoothAudioProvider::BluetoothAudioProvider()
    : death_recipient_(new BluetoothAudioDeathRecipient(this)),
      session_type_(SessionType::UNKNOWN),
      codec_config_({}) {
  codec_config_.pcmDataConfiguration = ::android::bluetooth::audio::
      BluetoothAudioSession::kInvalidPcmDataConfiguration;
}

Return<void> BluetoothAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  const PcmDataConfiguration pcmConfig = codecConfig.pcmDataConfiguration;
  if (hostIf == nullptr) {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
    return Void();
  } else if (pcmConfig.sampleRate == SampleRate::RATE_UNKNOWN ||
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
  stack_iface_->linkToDeath(death_recipient_, 0);

  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", PcmConfig=[" << toString(codec_config_.pcmDataConfiguration)
            << "], EncodedConfig=[{.codecType = "
            << toString(codec_config_.encodedDataConfiguration.codecType)
            << ", .peerMtu = " << codec_config_.encodedDataConfiguration.peerMtu
            << "}]";

  onSessionReady(_hidl_cb);
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
  if (stack_iface_) {
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
  if (stack_iface_) {
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

  if (stack_iface_) {
    BluetoothAudioSessionReport::OnSessionEnded(session_type_);
    stack_iface_->unlinkToDeath(death_recipient_);
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
  }

  /**
   * Clean up the audio platform as remote audio device is no
   * longer active
   */
  stack_iface_ = nullptr;
  codec_config_ = {};
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
