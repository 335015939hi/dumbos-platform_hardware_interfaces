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

using ::android::hardware::audio::common::V5_0::AudioContentType;
using ::android::hardware::audio::common::V5_0::AudioUsage;
using ::android::hardware::audio::common::V5_0::PlaybackTrackMetadata;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::TimeSpec;

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

static constexpr int kFmqSendTimeoutMs = 1000;  // 1000 ms timeout for sending
static constexpr int kWritePollMs = 1;          // polled non-blocking interval

static timespec timespec_convert_from_hal(const TimeSpec& TS);
static timespec timespec_convert_from_hal(const TimeSpec& TS) {
  return {.tv_sec = static_cast<long>(TS.tvSec),
          .tv_nsec = static_cast<long>(TS.tvNSec)};
}

BluetoothAudioSession::BluetoothAudioSession(const SessionType& session_type)
    : session_type_(session_type), stack_iface_(nullptr), mDataMQ(nullptr) {
  UpdatePcmDataConfig(kInvalidPcmDataConfiguration);
  UpdateEncodedDataConfig({});
}

// The API reports this session has been started successfully, and will invoke
// session_changed_cb_ to notify related outputs of the bluetooth_audio
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

// The API reports the Bluetooth stack has ended the session, and will invoke
// session_changed_cb_ to notify related outputs of the bluetooth_audio
void BluetoothAudioSession::OnSessionEnded() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    return;
  }
  ReportSessionStatus();
  UpdateCtrlPath(nullptr);
  UpdateDataPath(nullptr);
  UpdatePcmDataConfig(kInvalidPcmDataConfiguration);
  UpdateEncodedDataConfig({});
}

// private and internal function invokes the registered session_changed_cb_
void BluetoothAudioSession::ReportSessionStatus() {
  // This is locked already by OnSessionStarted / OnSessionEnded
  if (observers_.empty()) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " notify to bluetooth_audio=0x"
              << android::base::StringPrintf("%04x", cookie);
    cb->session_changed_cb_(cookie);
  }
}

// The API reports the Bluetooth stack has notified the result of startStream
// and suspendStream, and will invoke control_result_cb_ to notify related
// outputs of the bluetooth_audio
void BluetoothAudioSession::ReportControlStatus(
    bool start_resp, const BluetoothAudioStatus& status) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
    LOG(INFO) << __func__ << " - status=" << toString(status)
              << " for SessionType=" << toString(session_type_)
              << ", bluetooth_audio=0x"
              << android::base::StringPrintf("%04x", cookie)
              << (start_resp ? " started" : " suspended");
    cb->control_result_cb_(cookie, start_resp, status);
  }
}

// The common API helps to check if this session is ready or not
// @return: true if the Bluetooth stack has started the specified session
bool BluetoothAudioSession::IsSessionReady() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  bool dataMQ_valid =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH ||
       (mDataMQ != nullptr && mDataMQ->isValid()));
  return stack_iface_ != nullptr && dataMQ_valid;
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

// The control API helps the bluetooth_audio module to register
// PortStatusCallbacks
// @return: cookie - the assigned number to this output of the bluetooth_audio
uint16_t BluetoothAudioSession::RegisterStatusCback(
    const PortStatusCallbacks& cbacks) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  uint16_t cookie = ObserversCookieGetInitValue(session_type_);
  uint16_t cookie_upper_bound = ObserversCookieGetUpperBound(session_type_);

  while (cookie < cookie_upper_bound) {
    if (observers_.find(cookie) == observers_.end()) {
      break;
    }
    ++cookie;
  }
  if (cookie >= cookie_upper_bound) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " has " << observers_.size()
               << " observers already (No Resource)";
    return kObserversCookieUndefined;
  }
  std::shared_ptr<struct PortStatusCallbacks> cb =
      std::make_shared<struct PortStatusCallbacks>();
  *cb = cbacks;
  observers_[cookie] = cb;
  return cookie;
}

// The control API helps the bluetooth_audio module to unregister
// PortStatusCallbacks
// @param: cookie - indicates which output of the bluetooth_audio is
void BluetoothAudioSession::UnregisterStatusCback(uint16_t cookie) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!observers_.empty() && observers_.find(cookie) != observers_.end()) {
    observers_.erase(cookie);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " no such provider=0x"
                 << android::base::StringPrintf("%04x", cookie);
  }
}

// The control API for the bluetooth_audio module to the get current
// CodecConfiguration
const CodecConfiguration& BluetoothAudioSession::GetCodecConfig() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (IsSessionReady()) {
    return codec_config_;
  } else {
    return kInvalidCodecConfiguration;
  }
}

// The control APIs for the bluetooth_audio module to start / suspend / stop
// stream, to check position, and to update metadata.
bool BluetoothAudioSession::StartStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->startStream();
  if (hal_retval.isOk()) return true;
  LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
               << toString(session_type_) << " failed";
  return false;
}

bool BluetoothAudioSession::SuspendStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->suspendStream();
  if (hal_retval.isOk()) return true;
  LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
               << toString(session_type_) << " failed";
  return false;
}

void BluetoothAudioSession::StopStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) return;
  auto hal_retval = stack_iface_->stopStream();
  if (hal_retval.isOk()) return;
  LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
               << toString(session_type_) << " failed";
}

bool BluetoothAudioSession::GetPresentationPosition(
    uint64_t* remote_delay_report_ns, uint64_t* total_bytes_readed,
    timespec* data_position) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  bool retval = false;
  auto hal_retval = stack_iface_->getPresentationPosition(
      [&retval, &remote_delay_report_ns, &total_bytes_readed, &data_position](
          BluetoothAudioStatus status,
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
  if (hal_retval.isOk()) return retval;
  LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
               << toString(session_type_) << " failed";
  return false;
}

void BluetoothAudioSession::UpdateTracksMetadata(
    const struct source_metadata* source_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return;
  }

  ssize_t track_count = source_metadata->track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_) << ", "
            << track_count << " track(s)";
  if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
    return;
  }

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
                 << toString(session_type_) << " failed";
  }
}

// The control API writes stream to FMQ
size_t BluetoothAudioSession::OutWritePcmData(const void* buffer,
                                              size_t bytes) {
  if (buffer == nullptr || !bytes) return 0;
  size_t totalWritten = 0;
  int ms_timeout = kFmqSendTimeoutMs;
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
        return totalWritten;
      }
      totalWritten += availableToWrite;
    } else if (ms_timeout >= kWritePollMs) {
      lock.unlock();
      usleep(kWritePollMs * 1000);
      ms_timeout -= kWritePollMs;
    } else {
      ALOGD("data %zu/%zu overflow %d ms", totalWritten, bytes,
            (kFmqSendTimeoutMs - ms_timeout));
      return totalWritten;
    }
  } while (totalWritten < bytes);
  return totalWritten;
}

std::unique_ptr<BluetoothAudioSessionInstance>
    BluetoothAudioSessionInstance::instance_ptr =
        std::unique_ptr<BluetoothAudioSessionInstance>(
            new BluetoothAudioSessionInstance());

// API to fetch the session of A2DP / Hearing Aid
std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSessionInstance::GetSessionInstance(
    const SessionType& session_type) {
  std::lock_guard<std::mutex> guard(instance_ptr->mutex_);
  if (!instance_ptr->sessions_map_.empty()) {
    auto entry = instance_ptr->sessions_map_.find(session_type);
    if (entry != instance_ptr->sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession> session_ptr =
      std::make_shared<BluetoothAudioSession>(session_type);
  instance_ptr->sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
