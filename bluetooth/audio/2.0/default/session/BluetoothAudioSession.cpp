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
#define LOG_TAG "BTAudioProviderSession"

#include "BluetoothAudioSession.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

namespace android {
namespace bluetooth {
namespace audio {

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
#define FMQ_SEND_TIMEOUT_MS 1000 /* Timeout for sending */
#define WRITE_POLL_MS 1          // polled non-blocking interval

using ::android::hardware::audio::common::V5_0::AudioContentType;
using ::android::hardware::audio::common::V5_0::AudioUsage;
using ::android::hardware::audio::common::V5_0::PlaybackTrackMetadata;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::TimeSpec;

static timespec timespec_convert_from_hal(const TimeSpec& TS);
static timespec timespec_convert_from_hal(const TimeSpec& TS) {
  return {.tv_sec = static_cast<long>(TS.tvSec),
          .tv_nsec = static_cast<long>(TS.tvNSec)};
}

const CodecConfiguration BluetoothAudioSession::kInvalidCodecConfiguration = {
    .pcmDataConfiguration.sampleRate = SampleRate::RATE_UNKNOWN,
    .pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_UNKNOWN,
    .pcmDataConfiguration.channelMode = ChannelMode::UNKNOWN,
    .encodedDataConfiguration.codecType = CodecType::UNKNOWN,
    .encodedDataConfiguration.encodedAudioBitrate = 0x00000000,
    .encodedDataConfiguration.peerMtu = 0xffff,
    .encodedDataConfiguration.isScmstEnabled = false,
    .encodedDataConfiguration.codecSpecific = {}};

const PcmDataConfiguration&
    BluetoothAudioSession::kInvalidPcmDataConfiguration =
        BluetoothAudioSession::kInvalidCodecConfiguration.pcmDataConfiguration;

BluetoothAudioSession::BluetoothAudioSession(const SessionType& session_type)
    : session_type_(session_type), stack_iface_(nullptr), mDataMQ(nullptr) {
  UpdatePcmDataConfig(kInvalidPcmDataConfiguration);
  UpdateEncodedDataConfig({});
}

// API reports Bluetooth stack has started the session and will invoke
// session_changed_cb_ to notify related output of bluetooth_audio
void BluetoothAudioSession::OnSessionStarted(
    const sp<IBluetoothAudioPort> stack_iface, const DataMQ::Descriptor* dataMQ,
    const CodecConfiguration& codec_config) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!UpdateCtrlPath(stack_iface)) {
    LOG(ERROR) << __func__ << " - IBluetoothAudioPort invalid";
  } else if (!UpdateDataPath(dataMQ)) {
    LOG(ERROR) << __func__ << " - DataMQ invalid";
    UpdateCtrlPath(nullptr);
  } else if (!UpdatePcmDataConfig(codec_config.pcmDataConfiguration) ||
             !UpdateEncodedDataConfig(codec_config.encodedDataConfiguration)) {
    LOG(ERROR) << __func__ << " - CodecConfiguration invalid";
    UpdateCtrlPath(nullptr);
    UpdateDataPath(nullptr);
    UpdatePcmDataConfig(kInvalidPcmDataConfiguration);
    UpdateEncodedDataConfig({});
  } else {
    ReportSessionStatus();
  }
}

// API reports Bluetooth stack has ended the session and will invoke
// session_changed_cb_ to notify related output of bluetooth_audio
void BluetoothAudioSession::OnSessionEnded() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    UpdateCtrlPath(nullptr);
    UpdateDataPath(nullptr);
    UpdatePcmDataConfig(kInvalidPcmDataConfiguration);
    UpdateEncodedDataConfig({});
    ReportSessionStatus();
  }
}

// private and internal function invokes the registered session_changed_cb_
void BluetoothAudioSession::ReportSessionStatus() {
  // there is locked already by OnSessionStarted / OnSessionEnded
  if (!observers_.empty()) {
    for (auto& observer : observers_) {
      uint16_t cookies = observer.first;
      std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
      LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
                << " notify to bluetooth_audio=0x"
                << android::base::StringPrintf("%04x", cookies);
      cb->session_changed_cb_(cookies);
    }
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO port state observer";
  }
}

// API reports Bluetooth stack has notified the result of startStream /
// suspendStream and will invoke control_result_cb_ to notify related output
// of bluetooth_audio
void BluetoothAudioSession::ReportControlStatus(
    const bool& start_resp, const BluetoothAudioStatus& status) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!observers_.empty()) {
    for (auto& observer : observers_) {
      uint16_t cookies = observer.first;
      std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
      LOG(INFO) << __func__ << " - status=" << toString(status)
                << " for SessionType=" << toString(session_type_)
                << ", bluetooth_audio=0x"
                << android::base::StringPrintf("%04x", cookies)
                << (start_resp ? " started" : "suspended");
      cb->control_result_cb_(cookies, start_resp, status);
    }
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
  }
}

// private and internal function to check if session ready
bool BluetoothAudioSession::IsSessionReady() {
  return stack_iface_ != nullptr && (mDataMQ != nullptr && mDataMQ->isValid());
}

// private and internal function to setup / reset the audio control path
bool BluetoothAudioSession::UpdateCtrlPath(
    const sp<IBluetoothAudioPort> stack_iface) {
  stack_iface_ = stack_iface;
  return true;
}

// private and internal function to setup / reset the audio data path
bool BluetoothAudioSession::UpdateDataPath(const DataMQ::Descriptor* dataMQ) {
  if (dataMQ != nullptr) {
    std::unique_ptr<DataMQ> tempDataMQ;
    tempDataMQ.reset(new DataMQ(*dataMQ));
    if (tempDataMQ && tempDataMQ->isValid()) {
      mDataMQ = std::move(tempDataMQ);
      return true;
    } else {
      mDataMQ = nullptr;
      return false;
    }
  } else {
    mDataMQ = nullptr;
    // usecase of reset by nullptr
    return true;
  }
}

// private and internal function to setup / reset pcmDataConfiguration
bool BluetoothAudioSession::UpdatePcmDataConfig(
    const PcmDataConfiguration& pcm_config) {
  codec_config_.pcmDataConfiguration = pcm_config;
  return true;
}

// private and internal function to setup / reset encodedDataConfiguration
bool BluetoothAudioSession::UpdateEncodedDataConfig(
    const EncodedDataConfiguration& encoded_config) {
  codec_config_.encodedDataConfiguration = encoded_config;
  return true;
}

// Control API helps bluetooth_audio module to register PortStatusCallbacks
// return: cookies - the assigned number to this output of bluetooth_audio
uint16_t BluetoothAudioSession::RegisterStatusCback(
    const PortStatusCallbacks& cbacks) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  uint16_t cookies =
      (static_cast<uint16_t>(session_type_) << OBSERVERS_SESSION_TYPE_OFFSET_);
  uint16_t index = 0;

  while (index < OBSERVERS_SIZE_) {
    if (observers_.find(cookies) == observers_.end()) break;
    ++index;
    ++cookies;
  }
  if ((cookies & ~OBSERVERS_SESSION_TYPE_MASK_) >= OBSERVERS_SIZE_) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " has " << observers_.size()
               << " port state observers already";
    return OBSERVERS_COOKIES_UNDEF;
  }
  std::shared_ptr<struct PortStatusCallbacks> cb =
      std::make_shared<struct PortStatusCallbacks>();
  *cb = cbacks;
  observers_[cookies] = cb;
  return cookies;
}

// Control API helps bluetooth_audio module to unregister PortStatusCallbacks
// param: cookies - indicates which output of bluetooth_audio is
void BluetoothAudioSession::UnregisterStatusCback(const uint16_t& cookies) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!observers_.empty() && observers_.find(cookies) != observers_.end()) {
    observers_.erase(cookies);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " no such provider=0x"
                 << android::base::StringPrintf("%04x", cookies);
  }
}

// Control API for bluetooth_audio module to get current CodecConfiguration
const CodecConfiguration& BluetoothAudioSession::GetCodecConfig() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    return codec_config_;
  } else {
    return kInvalidCodecConfiguration;
  }
}

// Control APIs for bluetooth_audio module to start / suspend / stop stream,
// to check position, and to update metadata.
bool BluetoothAudioSession::StartStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    auto hal_retval = stack_iface_->startStream();
    if (hal_retval.isOk()) {
      return true;
    } else {
      LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                   << toString(session_type_) << " corrupted";
    }
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
  return false;
}

bool BluetoothAudioSession::SuspendStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    auto hal_retval = stack_iface_->suspendStream();
    if (hal_retval.isOk()) {
      return true;
    } else {
      LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                   << toString(session_type_) << " corrupted";
    }
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
  return false;
}

void BluetoothAudioSession::StopStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    auto hal_retval = stack_iface_->stopStream();
    if (!hal_retval.isOk()) {
      LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                   << toString(session_type_) << " corrupted";
    }
  }
}

bool BluetoothAudioSession::GetPresentationPosition(
    uint64_t* remote_delay_report_ns, uint64_t* total_bytes_readed,
    timespec* data_position) {
  bool retval = false;
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    auto hal_retval = stack_iface_->getPresentationPosition(
        [&](BluetoothAudioStatus status,
            const uint64_t& remoteDeviceAudioDelayNanos,
            uint64_t transmittedOctets,
            const TimeSpec& transmittedOctetsTimeStamp) {
          if (status == BluetoothAudioStatus::SUCCESS) {
            if (remote_delay_report_ns)
              *remote_delay_report_ns = remoteDeviceAudioDelayNanos;
            if (total_bytes_readed) *total_bytes_readed = transmittedOctets;
            if (data_position)
              *data_position =
                  timespec_convert_from_hal(transmittedOctetsTimeStamp);
            retval = true;
          }
        });
    if (!hal_retval.isOk()) {
      LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                   << toString(session_type_) << " corrupted";
    }
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
  if (!retval) {
    if (remote_delay_report_ns) *remote_delay_report_ns = 0;
    if (total_bytes_readed) *total_bytes_readed = 0;
    if (data_position) *data_position = {};
  }
  return retval;
}

void BluetoothAudioSession::UpdateTracksMetadata(
    const struct source_metadata* source_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    ssize_t track_count = source_metadata->track_count;

    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << ", " << track_count << " track(s)";

    if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
        session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH)
      return;

    struct playback_track_metadata* track = source_metadata->tracks;
    SourceMetadata sourceMetadata;
    PlaybackTrackMetadata* halMetadata;

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
    auto hal_retval = stack_iface_->updateMetadata(sourceMetadata);
    if (!hal_retval.isOk()) {
      LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                   << toString(session_type_) << " corrupted";
    }
  } else {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
  }
}

// Control API to write stream to FMQ
size_t BluetoothAudioSession::OutWritePcmData(const void* buffer,
                                              size_t bytes) {
  if (buffer == nullptr || !bytes) return 0;
  size_t totalWritten = 0;
  int ms_timeout = FMQ_SEND_TIMEOUT_MS;
  do {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (!IsSessionReady()) break;
    size_t availableToWrite = mDataMQ->availableToWrite();
    if (availableToWrite) {
      if (availableToWrite > (bytes - totalWritten)) {
        availableToWrite = bytes - totalWritten;
      }

      if (!mDataMQ->write(static_cast<const uint8_t*>(buffer) + totalWritten,
                          availableToWrite)) {
        ALOGE("FMQ datapath writting %zu/%zu failed", totalWritten, bytes);
        break;
      } else {
        totalWritten += availableToWrite;
      }
    } else if (ms_timeout >= WRITE_POLL_MS) {
      lock.unlock();
      usleep(WRITE_POLL_MS * 1000);
      ms_timeout -= WRITE_POLL_MS;
    } else {
      ALOGD("data %zu/%zu overflow %d ms", totalWritten, bytes,
            (FMQ_SEND_TIMEOUT_MS - ms_timeout));
      break;
    }
  } while (totalWritten < bytes);
  return totalWritten;
}

std::unique_ptr<BluetoothAudioSessionInstance>
    BluetoothAudioSessionInstance::instance_ptr_;
std::unordered_map<SessionType, std::shared_ptr<BluetoothAudioSession>>
    BluetoothAudioSessionInstance::sessions_map_;

// API to fetch the session of A2DP / Hearing Aid
std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSessionInstance::GetSessionInstance(
    const SessionType& session_type) {
  if (instance_ptr_ == nullptr) {
    instance_ptr_ = std::unique_ptr<BluetoothAudioSessionInstance>(
        new BluetoothAudioSessionInstance());
    LOG(DEBUG) << __func__
               << " - BluetoothAudioSessionInstance=" << instance_ptr_
               << " allocated";
  }

  if (!instance_ptr_->sessions_map_.empty()) {
    auto entry = instance_ptr_->sessions_map_.find(session_type);
    if (entry != instance_ptr_->sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession> session_ptr =
      std::make_shared<BluetoothAudioSession>(session_type);
  instance_ptr_->sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
