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

#include <log/log.h>

#include "BluetoothAudioProvider.h"

#ifndef PCM_FRAME_SIZE
#define PCM_FRAME_SIZE   4    // stereo
#endif /* PCM_FRAME_SIZE */
#ifndef PCM_FRAME_COUNT
#define PCM_FRAME_COUNT  128
#endif /* PCM_FRAME_COUNT */

#ifndef RTP_FRAME_SIZE
#define RTP_FRAME_SIZE   (PCM_FRAME_SIZE * PCM_FRAME_COUNT)
#endif /* RTP_FRAME_SIZE */
#ifndef RTP_FRAME_COUNT
#define RTP_FRAME_COUNT  7    /* full of size within 1 tick (20ms) */
#endif /* RTP_FRAME_COUNT */

#define BUFFER_SIZE      (RTP_FRAME_SIZE * RTP_FRAME_COUNT)
//#define DOUBLE_BUFFER    1    /* single buffer */
#define DOUBLE_BUFFER    2    /* double buffer */
//#define DOUBLE_BUFFER    3    /* triple buffer */


namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::audio::common::V5_0::AudioContentType;
using android::hardware::audio::common::V5_0::AudioUsage;
using android::hardware::audio::common::V5_0::PlaybackTrackMetadata;
using android::hardware::audio::common::V5_0::SourceMetadata;
using android::hardware::hidl_death_recipient;
using android::hardware::Void;

using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::TimeSpec;

const PcmDataConfiguration BluetoothAudioProvider::kInvalidPcmConfiguration = {
    .sampleRate = SampleRate::RATE_UNKNOWN,
    .bitsPerSample = BitsPerSample::BITS_UNKNOWN,
    .channelMode = ChannelMode::UNKNOWN};

BluetoothAudioProvider::BluetoothAudioProvider()
    : session_type_(SessionType::UNKNOWN), has_session_(false),
      death_recipient_(new BluetoothAudioDeathRecipient(this)),
      unlink_cb_(nullptr) {

  // FIXME: which number of frameSize and framesCount should we use?
  ALOGI("%s - size of audio data path %d byte(s)", __func__, BUFFER_SIZE*DOUBLE_BUFFER);
  std::unique_ptr<DataMQ> tempDataMQ(new DataMQ(BUFFER_SIZE*DOUBLE_BUFFER, /* EventFlag */ true));
  if (!tempDataMQ->isValid()) {
    ALOGE_IF(!tempDataMQ->isValid(), "data MQ is invalid");
  } else {
    mDataMQ = std::move(tempDataMQ);
  }
}

// TODO
//BluetoothAudioProvider::~BluetoothAudioProvider() {
//}

bool BluetoothAudioProvider::build(const SessionType& sessionType) {
  // BluetoothAudioProviderSub is well to support all types
  session_type_ = sessionType;
  return true;
}

Return<void> BluetoothAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  BluetoothAudioStatus result = BluetoothAudioStatus::SUCCESS;

  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */

  codec_config_ = codecConfig;
  stack_iface_ = hostIf;
  has_session_ = true;

  ALOGI("%s - SessionType=%s(0x%02hhx), PcmConfig=[%s], "
        "EncodedConfig=[{.codecType = %s, .peerMtu = 0x%04x}]", __func__,
        toString(session_type_).c_str(), session_type_,
        toString(codec_config_.pcmDataConfiguration).c_str(),
        toString(codec_config_.encodedDataConfiguration.codecType).c_str(),
        codec_config_.encodedDataConfiguration.peerMtu);

  death_recipient_->setHasDied(false);
  hostIf->linkToDeath(death_recipient_, 0);

  unlink_cb_ = [hostIf](sp<BluetoothAudioDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      ALOGI("Skipping unlink call, service died.");
    else
      hostIf->unlinkToDeath(death_recipient);
  };

  _hidl_cb(result, *mDataMQ->getDesc());
  return Void();
}

Return<void> BluetoothAudioProvider::streamStarted(BluetoothAudioStatus status) {

  ALOGI("%s - status=%s(0x%02hhx)", __func__, toString(status).c_str(), status);

  /**
   * Streaming on control path has started,
   * HAL server should start the streaming on data path.
   */
  if (has_session_) {
    std::lock_guard<std::mutex> guard(internal_mutex_);
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        struct PortStateCback *cb = observer.second;
        ALOGD("%s - status=%s(0x%02hhx) for SessionType=%s(0x%02hhx), ctrl_key=0x%04hx",
              __func__, toString(status).c_str(), status,
              toString(session_type_).c_str(), session_type_, cb->ctrl_key);
        cb->ctrl_res_cb_(cb->ctrl_key, status);
      }
    } else {
      ALOGW("%s - SessionType=%s(0x%02hhx) has NO observer", __func__,
            toString(session_type_).c_str(), session_type_);
    }
  } else {
    ALOGW("%s - provider has no session: status=%s(0x%02hhx)", __func__,
          toString(status).c_str(), status);
  }

  return Void();
}

Return<void> BluetoothAudioProvider::streamSuspended(BluetoothAudioStatus status) {

  ALOGI("%s - status=%s(0x%02hhx)", __func__, toString(status).c_str(), status);

  /**
   * Streaming on control path has suspend,
   * HAL server should suspend the streaming on data path.
   */
  if (has_session_) {
    std::lock_guard<std::mutex> guard(internal_mutex_);
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        struct PortStateCback *cb = observer.second;
        ALOGD("%s - status=%s(0x%02hhx) for SessionType=%s(0x%02hhx), ctrl_key=0x%04hx",
              __func__, toString(status).c_str(), status,
              toString(session_type_).c_str(), session_type_, cb->ctrl_key);
        cb->ctrl_res_cb_(cb->ctrl_key, status);
      }
    } else {
      ALOGW("%s - SessionType=%s(0x%02hhx) has NO observer", __func__,
            toString(session_type_).c_str(), session_type_);
    }
  } else {
    ALOGW("%s - provider has no session: status=%s(0x%02hhx)", __func__,
          toString(status).c_str(), status);
  }

  return Void();
}

Return<void> BluetoothAudioProvider::endSession() {

  std::lock_guard<std::mutex> guard(internal_mutex_);

  ALOGI("%s", __func__);

  if (unlink_cb_) {
    unlink_cb_(death_recipient_);
    death_recipient_->setHasDied(true);
    unlink_cb_ = nullptr;
  } else {
    ALOGW("%s - provider session ended twice", __func__);
  }

  if (has_session_) {
    if (!observers_.empty()) {
      for (auto& observer : observers_) {
        struct PortStateCback *cb = observer.second;
        ALOGD("%s - notify to SessionType=%s(0x%02hhx), ctrl_key=0x%04hx",
              __func__, toString(session_type_).c_str(), session_type_,
              cb->ctrl_key);
        cb->session_ended_cb_(cb->ctrl_key);
        delete cb;
      }
    } else {
      ALOGW("%s - SessionType=%s(0x%02hhx) has NO observer", __func__,
            toString(session_type_).c_str(), session_type_);
    }
  } else {
    ALOGW("%s - provider has NO session", __func__);
  }
  observers_.clear();

  /**
   * Cleanup the audio platform as remote audio device is no
   * longer active
   */
  has_session_ = false;
  // FIXME: should we reset the SessionType??
  //session_type_ = SessionType::UNKNOWN;
  stack_iface_ = nullptr;

  codec_config_.pcmDataConfiguration = kInvalidPcmConfiguration;

  return Void();
}

// Extra APIs for Audio HW module: bluetooth_audio
const DataMQ::Descriptor* BluetoothAudioProvider::getStreamDataFMQ() {
  if (has_session_) {
    if (mDataMQ != nullptr && mDataMQ->isValid()) {
      return mDataMQ->getDesc();
    } else {
      ALOGW("%s - SessionType=%s(0x%02hhx), mDataMQ=%p invalid", __func__,
            toString(session_type_).c_str(), session_type_,
            (mDataMQ ? mDataMQ.get() : nullptr));
    }
  } else {
    ALOGW("%s - Provider has NO session", __func__);
  }
  return nullptr;
}

const PcmDataConfiguration& BluetoothAudioProvider::getStreamPcmDataConfig(void) {
  PcmDataConfiguration& pcmConfig = codec_config_.pcmDataConfiguration;;
  if (!has_session_) {
    ALOGW("%s - Provider has NO session", __func__);
    pcmConfig = kInvalidPcmConfiguration;
  }
  return pcmConfig;
}

const sp<IBluetoothAudioPort> BluetoothAudioProvider::getAssociatedPortCtrl() {
  if (has_session_)
    return stack_iface_;
  else
    ALOGW("%s - provider has NO session", __func__);
  return nullptr;
}

bool BluetoothAudioProvider::getStreamPresentationPosition(
    timespec &remote_delay_report, uint64_t &total_bytes_readed,
    timespec &data_position) {
  bool retval = false;
  if (has_session_ && stack_iface_) {
    auto halRetval = stack_iface_->getPresentationPosition(
        [&](BluetoothAudioStatus status,
            const TimeSpec& remoteDeviceAudioDelay,
            uint64_t transmittedOctets,
            const TimeSpec& transmittedOctetsTimeStamp) {
          if (status == BluetoothAudioStatus::SUCCESS) {
            remote_delay_report.tv_sec = remoteDeviceAudioDelay.tvSec;
            remote_delay_report.tv_nsec = remoteDeviceAudioDelay.tvNSec;
            total_bytes_readed = transmittedOctets;
            data_position.tv_sec = transmittedOctetsTimeStamp.tvSec;
            data_position.tv_nsec = transmittedOctetsTimeStamp.tvNSec;
            retval = true;
          }
        });
    if (!halRetval.isOk())
      ALOGW("%s - getPresentationPosition failure", __func__);
  } else {
    ALOGW("%s - provider has NO session", __func__);
  }
  if (!retval) {
    remote_delay_report = {};
    total_bytes_readed = 0;
    data_position = {};
  }
  return retval;
}

void BluetoothAudioProvider::updateStreamTracksMetadata(
    const struct source_metadata *source_metadata) {
  if (has_session_ && stack_iface_) {
    ssize_t track_count = source_metadata->track_count;
    struct playback_track_metadata *track = source_metadata->tracks;
    SourceMetadata sourceMetadata;
    PlaybackTrackMetadata *halMetadata;

    ALOGD("%s - SessionType=%s(0x%02hhx), %zu track(s)", __func__,
          toString(session_type_).c_str(), session_type_, track_count);
    sourceMetadata.tracks.resize(track_count);
    halMetadata = sourceMetadata.tracks.data();
    while (track_count && track) {
      halMetadata->usage = static_cast<AudioUsage>(track->usage);
      halMetadata->contentType = static_cast<AudioContentType>(track->content_type);
      halMetadata->gain = track->gain;
      ALOGV("%s - SessionType=%s(0x%02hhx), usage=%d, content=%d, gain=%f",
            __func__, toString(session_type_).c_str(), session_type_,
            halMetadata->usage, halMetadata->contentType, halMetadata->gain);
      --track_count;
      ++track;
      ++halMetadata;
    }
    auto halRetval = stack_iface_->updateMetadata(sourceMetadata);
    if (!halRetval.isOk())
      ALOGE("%s - SessionType=%s(0x%02hhx) failure", __func__,
             toString(session_type_).c_str(), session_type_);
  } else
    ALOGW("%s - provider has NO session", __func__);
}

uint16_t BluetoothAudioProvider::registerControlResultCback(
    std::function<void(const uint16_t&, const BluetoothAudioStatus&)>& ctrl_res_cb,
    std::function<void(const uint16_t&)>& session_ended_cb) {
  uint16_t ctrl_key = (static_cast<uint16_t>(session_type_) << OBSERVERS_SESSION_TYPE_OFFSET_);
  uint16_t idx = 0;

  if (!has_session_) {
    ALOGW("%s - provider has NO session", __func__);
    return OBSERVERS_CTRL_KEY_UNDEF;
  } else if (!ctrl_res_cb || !session_ended_cb) {
    ALOGE("%s - invalid cback for port state observer", __func__);
    return OBSERVERS_CTRL_KEY_UNDEF;
  }

  std::lock_guard<std::mutex> guard(internal_mutex_);
  while (idx < OBSERVERS_SIZE_) {
    if (observers_.find(ctrl_key) == observers_.end())
      break;
    ++ctrl_key;
    ++idx;
  }

  if ((ctrl_key & ~OBSERVERS_SESSION_TYPE_MASK_) >= OBSERVERS_SIZE_) {
    ALOGE("%s - provider has %zu observers already", __func__, observers_.size());
    return OBSERVERS_CTRL_KEY_UNDEF;
  }

  struct PortStateCback *cb = new PortStateCback();
  cb->ctrl_key = ctrl_key;
  cb->ctrl_res_cb_ = ctrl_res_cb;
  cb->session_ended_cb_ = session_ended_cb;
  observers_[cb->ctrl_key] = cb;

  return ctrl_key;
}

Return<void> BluetoothAudioProvider::unregisterControlResultCback(uint16_t& ctrl_key) {
  if (session_type_ != OBSERVERS_GET_SESSION_TYPE(ctrl_key)) {
    ALOGW("%s - wrong SessionType=%s(0x%02hhx):%s(0x%02hhx)", __func__,
          toString(session_type_).c_str(), session_type_,
          toString(OBSERVERS_GET_SESSION_TYPE(ctrl_key)).c_str(),
          OBSERVERS_GET_SESSION_TYPE(ctrl_key));
    return Void();
  }

  std::lock_guard<std::mutex> guard(internal_mutex_);
  if (observers_.find(ctrl_key) != observers_.end()) {
    auto observer = observers_.find(ctrl_key);
    struct PortStateCback *cb = observer->second;
    delete cb;
    observers_.erase(ctrl_key);
    ctrl_key = OBSERVERS_CTRL_KEY_UNDEF;
  } else {
    ALOGW("%s - no such ctrl cback", __func__);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
