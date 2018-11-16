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

#define LOG_TAG "BTAudioProviderStub"

#include "BluetoothAudioProvider.h"

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

using ::android::hardware::hidl_death_recipient;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V5_0::AudioContentType;
using ::android::hardware::audio::common::V5_0::AudioUsage;
using ::android::hardware::audio::common::V5_0::PlaybackTrackMetadata;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::TimeSpec;

const PcmDataConfiguration BluetoothAudioProvider::kInvalidPcmConfiguration = {
    .sampleRate = SampleRate::RATE_UNKNOWN,
    .bitsPerSample = BitsPerSample::BITS_UNKNOWN,
    .channelMode = ChannelMode::UNKNOWN};

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

  std::lock_guard<std::mutex> guard(internal_mutex_);
  if (!observers_.empty()) {
    for (auto& observer : observers_) {
      shared_ptr<struct PortStateCback> cb = observer.second;
      LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
                << " notify to provider=0x"
                << android::base::StringPrintf("%04x", cb->ctrl_key);
      cb->session_changed_cb_(cb->ctrl_key);
    }
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO port state observer";
  }

  if (mDataMQ && mDataMQ->isValid()) {
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    // A2DP_HARDWARE_OFFLOAD_DATAPATH
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
    std::lock_guard<std::mutex> guard(internal_mutex_);
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        shared_ptr<struct PortStateCback> cb = observer.second;
        LOG(VERBOSE) << __func__ << " - status=" << toString(status)
                     << " for SessionType=" << toString(session_type_)
                     << ", provider=0x"
                     << android::base::StringPrintf("%04x", cb->ctrl_key);
        cb->ctrl_res_cb_(cb->ctrl_key, status);
      }
    } else {
      LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                   << " has NO port state observer";
    }
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
    std::lock_guard<std::mutex> guard(internal_mutex_);
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        shared_ptr<struct PortStateCback> cb = observer.second;
        LOG(VERBOSE) << __func__ << " - status=" << toString(status)
                     << " for SessionType=" << toString(session_type_)
                     << ", provider=0x"
                     << android::base::StringPrintf("%04x", cb->ctrl_key);
        cb->ctrl_res_cb_(cb->ctrl_key, status);
      }
    } else {
      LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                   << " has NO port state observer";
    }
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
    std::lock_guard<std::mutex> guard(internal_mutex_);
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        shared_ptr<struct PortStateCback> cb = observer.second;
        LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
                  << " notify to provider=0x"
                  << android::base::StringPrintf("%04x", cb->ctrl_key);
        cb->session_changed_cb_(cb->ctrl_key);
      }
    } else {
      LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
                << " has NO port state observer";
    }
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

  codec_config_.pcmDataConfiguration = kInvalidPcmConfiguration;

  return Void();
}

// Extra APIs for Audio HW module: fetch the audio data path
const DataMQ::Descriptor* BluetoothAudioProvider::getStreamDataFMQ() {
  if (has_session_) {
    if (mDataMQ != nullptr && mDataMQ->isValid()) {
      return mDataMQ->getDesc();
    } else {
      LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", mDataMQ=" << (mDataMQ ? mDataMQ.get() : nullptr)
                 << " invalid";
    }
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO session";
  }
  return nullptr;
}

// Extra APIs for Audio HW module: fetch the audio control path
const sp<IBluetoothAudioPort> BluetoothAudioProvider::getAssociatedPortCtrl() {
  if (has_session_) {
    return stack_iface_;
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO session";
    return nullptr;
  }
}

// Extra APIs for Audio HW module: fetch the audio PCM configuration
const PcmDataConfiguration& BluetoothAudioProvider::getStreamPcmDataConfig() {
  if (has_session_) {
    return codec_config_.pcmDataConfiguration;
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO session";
    return kInvalidPcmConfiguration;
  }
}

static timespec timespec_convert_from_hal(const TimeSpec& TS);
static timespec timespec_convert_from_hal(const TimeSpec& TS) {
  return {.tv_sec = static_cast<long>(TS.tvSec),
          .tv_nsec = static_cast<long>(TS.tvNSec)};
}

bool BluetoothAudioProvider::getStreamPresentationPosition(
    timespec& remote_delay_report, uint64_t& total_bytes_readed,
    timespec& data_position) {
  bool retval = false;
  if (has_session_ && stack_iface_) {
    auto halRetval = stack_iface_->getPresentationPosition(
        [&](BluetoothAudioStatus status, const TimeSpec& remoteDeviceAudioDelay,
            uint64_t transmittedOctets,
            const TimeSpec& transmittedOctetsTimeStamp) {
          if (status == BluetoothAudioStatus::SUCCESS) {
            remote_delay_report =
                timespec_convert_from_hal(remoteDeviceAudioDelay);
            total_bytes_readed = transmittedOctets;
            data_position =
                timespec_convert_from_hal(transmittedOctetsTimeStamp);
            retval = true;
          }
        });
    if (!halRetval.isOk())
      LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                   << " FAILURE";
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
  if (!retval) {
    remote_delay_report = {};
    total_bytes_readed = 0;
    data_position = {};
  }
  return retval;
}

void BluetoothAudioProvider::updateStreamTracksMetadata(
    const struct source_metadata* source_metadata) {
  if (has_session_ && stack_iface_) {
    ssize_t track_count = source_metadata->track_count;
    struct playback_track_metadata* track = source_metadata->tracks;
    SourceMetadata sourceMetadata;
    PlaybackTrackMetadata* halMetadata;

    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << ", " << track_count << " track(s)";
    sourceMetadata.tracks.resize(track_count);
    halMetadata = sourceMetadata.tracks.data();
    while (track_count && track) {
      halMetadata->usage = static_cast<AudioUsage>(track->usage);
      halMetadata->contentType =
          static_cast<AudioContentType>(track->content_type);
      halMetadata->gain = track->gain;
      LOG(VERBOSE) << __func__ << " - SessionType=" << toString(session_type_)
                   << ", usage=" << toString(halMetadata->usage)
                   << ", content=" << toString(halMetadata->contentType)
                   << ", gain=" << halMetadata->gain;
      --track_count;
      ++track;
      ++halMetadata;
    }
    auto halRetval = stack_iface_->updateMetadata(sourceMetadata);
    if (!halRetval.isOk())
      LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                   << " FAILURE";
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
}

uint16_t BluetoothAudioProvider::registerControlResultCback(
    std::function<void(const uint16_t&, const BluetoothAudioStatus&)>&
        ctrl_res_cb,
    std::function<void(const uint16_t&)>& session_changed_cb) {
  uint16_t ctrl_key =
      (static_cast<uint16_t>(session_type_) << OBSERVERS_SESSION_TYPE_OFFSET_);
  uint16_t idx = 0;

  if (!has_session_) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO session";
    return OBSERVERS_CTRL_KEY_UNDEF;
  } else if (!ctrl_res_cb || !session_changed_cb) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " invalid cback for port state observer";
    return OBSERVERS_CTRL_KEY_UNDEF;
  }

  std::lock_guard<std::mutex> guard(internal_mutex_);
  while (idx < OBSERVERS_SIZE_) {
    if (observers_.find(ctrl_key) == observers_.end()) break;
    ++ctrl_key;
    ++idx;
  }

  if ((ctrl_key & ~OBSERVERS_SESSION_TYPE_MASK_) >= OBSERVERS_SIZE_) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " has " << observers_.size()
               << " port state observers already";
    return OBSERVERS_CTRL_KEY_UNDEF;
  }

  shared_ptr<struct PortStateCback> cb =
      std::make_shared<struct PortStateCback>();
  cb->ctrl_key = ctrl_key;
  cb->ctrl_res_cb_ = ctrl_res_cb;
  cb->session_changed_cb_ = session_changed_cb;
  observers_[cb->ctrl_key] = cb;

  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", provider=0x"
            << android::base::StringPrintf("%04x", cb->ctrl_key);
  return ctrl_key;
}

Return<void> BluetoothAudioProvider::unregisterControlResultCback(
    uint16_t& ctrl_key) {
  if (session_type_ != OBSERVERS_GET_SESSION_TYPE(ctrl_key)) {
    LOG(WARNING) << __func__
                 << " - wrong SessionType=" << toString(session_type_) << "(0x"
                 << android::base::StringPrintf("%02hhx", session_type_)
                 << "):" << toString(OBSERVERS_GET_SESSION_TYPE(ctrl_key))
                 << "(0x"
                 << android::base::StringPrintf(
                        "%02hhx", OBSERVERS_GET_SESSION_TYPE(ctrl_key))
                 << ")";
    return Void();
  }

  std::lock_guard<std::mutex> guard(internal_mutex_);
  if (observers_.find(ctrl_key) != observers_.end()) {
    observers_.erase(ctrl_key);
    ctrl_key = OBSERVERS_CTRL_KEY_UNDEF;
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " no such provider=0x"
                 << android::base::StringPrintf("%04x", ctrl_key);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
