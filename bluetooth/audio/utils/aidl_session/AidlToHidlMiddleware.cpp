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
#include <unordered_map>

#include "../aidl_session/BluetoothAudioSessionControl.h"
#include "../session/BluetoothAudioSessionControl_2_1.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using SessionControl_2_1 =
    ::android::bluetooth::audio::BluetoothAudioSessionControl_2_1;
using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;

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

bool AidlToHidlMiddleware::IsSessionReady(const SessionType& session_type) {
  return SessionControl_2_1::IsSessionReady(to_hidl_session_type(session_type));
}

// uint16_t AidlToHidlMiddleware::RegisterControlResultCback(
//     const SessionType& session_type, const PortStatusCallbacks& cbacks) {}

// void AidlToHidlMiddleware::UnregisterControlResultCback(
//     const SessionType& session_type, uint16_t cookie) {}

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