/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "BtAudioNakahara"

#include "AidlToHidlMiddleware.h"

#include <android-base/logging.h>
#include <android/hardware/bluetooth/audio/2.1/types.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "../aidl_session/BluetoothAudioSessionControl.h"
#include "../session/BluetoothAudioSessionControl_2_1.h"
#include "BluetoothAudioSession.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using SessionControl_2_1 =
    ::android::bluetooth::audio::BluetoothAudioSessionControl_2_1;
using PortStatusCallbacksHidl =
    ::android::bluetooth::audio::PortStatusCallbacks;
using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;
using HidlStatus = ::android::hardware::bluetooth::audio::V2_0::Status;

std::mutex callback_lock;
std::unordered_map<uint16_t, std::shared_ptr<PortStatusCallbacks>>
    callback_table;

const static std::unordered_map<SessionType, SessionType_2_1>
    session_type_to_hidl_map{
        {SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
         SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH},
        {SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::HEARING_AID_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_SOFTWARE_DECODED_DATAPATH},
        {SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH},
    };

inline SessionType_2_1 to_hidl_session_type(
    const SessionType& session_type_aidl) {
  auto it = session_type_to_hidl_map.find(session_type_aidl);
  if (it != session_type_to_hidl_map.end()) return it->second;
  return SessionType_2_1::UNKNOWN;
}

inline BluetoothAudioStatus to_aidl_status(const HidlStatus& status) {
  switch (status) {
    case HidlStatus::SUCCESS:
      return BluetoothAudioStatus::SUCCESS;
    case HidlStatus::UNSUPPORTED_CODEC_CONFIGURATION:
      return BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION;
    default:
      return BluetoothAudioStatus::FAILURE;
  }
}

bool AidlToHidlMiddleware::IsSessionReady(const SessionType& session_type) {
  return SessionControl_2_1::IsSessionReady(to_hidl_session_type(session_type));
}

static void control_status_callback(uint16_t cookie, bool start_resp,
                                    const HidlStatus& status) {
  auto it = callback_table.find(cookie);
  if (it == callback_table.end()) {
    return;
  }
  auto cbacks = it->second;
  if (cbacks->control_result_cb_)
    cbacks->control_result_cb_(cookie, start_resp, to_aidl_status(status));
}

static void session_changed_cb(uint16_t cookie) {
  auto it = callback_table.find(cookie);
  if (it == callback_table.end()) {
    return;
  }
  auto cbacks = it->second;
  if (cbacks->session_changed_cb_) cbacks->session_changed_cb_(cookie);
}

PortStatusCallbacksHidl port_status_callbacks = {
    .control_result_cb_ = control_status_callback,
    .session_changed_cb_ = session_changed_cb,
};

uint16_t AidlToHidlMiddleware::RegisterControlResultCback(
    const SessionType& session_type, const PortStatusCallbacks& cbacks) {
  LOG(INFO) << __func__ << ": " << toString(session_type);

  auto cookie = SessionControl_2_1::RegisterControlResultCback(
      to_hidl_session_type(session_type), port_status_callbacks);
  {
    std::lock_guard<std::mutex> guard(callback_lock);
    callback_table[cookie] = std::make_shared<PortStatusCallbacks>(cbacks);
  }
  return cookie;
}

void AidlToHidlMiddleware::UnregisterControlResultCback(
    const SessionType& session_type, uint16_t cookie) {
  LOG(INFO) << __func__ << ": " << toString(session_type);
  SessionControl_2_1::UnregisterControlResultCback(
      to_hidl_session_type(session_type), cookie);

  {
    std::lock_guard<std::mutex> guard(callback_lock);
    auto it = callback_table.find(cookie);
    if (it != callback_table.end()) {
      callback_table.erase(it);
    }
  }
}

// const AudioConfiguration AidlToHidlMiddleware::GetAudioConfig(
//     const SessionType& session_type) {}

bool AidlToHidlMiddleware::StartStream(const SessionType& session_type) {
  return SessionControl_2_1::StartStream(to_hidl_session_type(session_type));
}

void AidlToHidlMiddleware::StopStream(const SessionType& session_type) {
  return SessionControl_2_1::StopStream(to_hidl_session_type(session_type));
}

bool AidlToHidlMiddleware::SuspendStream(const SessionType& session_type) {
  return SessionControl_2_1::SuspendStream(to_hidl_session_type(session_type));
}

bool AidlToHidlMiddleware::GetPresentationPosition(
    const SessionType& session_type,
    PresentationPosition& presentation_position) {
  uint64_t remote_delay_report_ns;
  uint64_t total_bytes_readed;
  timespec data_position;
  auto ret_val = SessionControl_2_1::GetPresentationPosition(
      to_hidl_session_type(session_type), &remote_delay_report_ns,
      &total_bytes_readed, &data_position);

  presentation_position = {
      .remoteDeviceAudioDelayNanos =
          static_cast<int64_t>(remote_delay_report_ns),
      .transmittedOctets = static_cast<int64_t>(total_bytes_readed),
      .transmittedOctetsTimestamp =
          {
              .tvSec = static_cast<int64_t>(data_position.tv_sec),
              .tvNSec = static_cast<int64_t>(data_position.tv_nsec),
          },
  };
  return ret_val;
}

void AidlToHidlMiddleware::UpdateSourceMetadata(
    const SessionType& session_type,
    const struct source_metadata* source_metadata) {
  return SessionControl_2_1::UpdateTracksMetadata(
      to_hidl_session_type(session_type), source_metadata);
}

void AidlToHidlMiddleware::UpdateSinkMetadata(const SessionType&,
                                              const struct sink_metadata&) {
  LOG(ERROR) << __func__ << " not supported in HIDL";
}

size_t AidlToHidlMiddleware::OutWritePcmData(const SessionType& session_type,
                                             const void* buffer, size_t bytes) {
  return SessionControl_2_1::OutWritePcmData(to_hidl_session_type(session_type),
                                             buffer, bytes);
}

size_t AidlToHidlMiddleware::InReadPcmData(const SessionType& session_type,
                                           void* buffer, size_t bytes) {
  return SessionControl_2_1::InReadPcmData(to_hidl_session_type(session_type),
                                           buffer, bytes);
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl