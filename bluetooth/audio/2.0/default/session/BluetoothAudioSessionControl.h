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

#ifndef BLUETOOTH_AUDIO_SESSION_CONTROL_H_
#define BLUETOOTH_AUDIO_SESSION_CONTROL_H_

#include "BluetoothAudioSession.h"

namespace android {
namespace bluetooth {
namespace audio {

class BluetoothAudioSessionControl {
 public:
  // Control API helps bluetooth_audio module to register PortStatusCallbacks
  // return: cookies - the assigned number to this output of bluetooth_audio
  static uint16_t RegisterControlResultCback(
      const SessionType& session_type, const PortStatusCallbacks& cbacks) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->RegisterStatusCback(cbacks);
    }
    return OBSERVERS_COOKIES_UNDEF;
  }

  // Control API helps bluetooth_audio module to unregister PortStatusCallbacks
  // param: cookies - indicates which output of bluetooth_audio is
  static void UnregisterControlResultCback(const SessionType& session_type,
                                           const uint16_t& cookies) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      session_ptr->UnregisterStatusCback(cookies);
    }
  }

  // Control API for bluetooth_audio module to get current CodecConfiguration
  static const CodecConfiguration& GetCodecConfig(
      const SessionType& session_type) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->GetCodecConfig();
    }
    return BluetoothAudioSession::kInvalidCodecConfiguration;
  }

  // Control APIs for bluetooth_audio module to start / suspend / stop stream,
  // to check position, and to update metadata.
  static bool StartStream(const SessionType& session_type) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->StartStream();
    }
    return false;
  }

  static bool SuspendStream(const SessionType& session_type) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->SuspendStream();
    }
    return false;
  }

  static void StopStream(const SessionType& session_type) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      session_ptr->StopStream();
    }
  }

  static bool GetPresentationPosition(const SessionType& session_type,
                                      uint64_t* remote_delay_report_ns,
                                      uint64_t* total_bytes_readed,
                                      timespec* data_position) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->GetPresentationPosition(
          remote_delay_report_ns, total_bytes_readed, data_position);
    }
    return false;
  }

  static void UpdateTracksMetadata(
      const SessionType& session_type,
      const struct source_metadata* source_metadata) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      session_ptr->UpdateTracksMetadata(source_metadata);
    }
  }

  // Control API to write stream to FMQ
  static size_t OutWritePcmData(const SessionType& session_type,
                                const void* buffer, size_t bytes) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->OutWritePcmData(buffer, bytes);
    }
    return 0;
  }
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android

#endif  // BLUETOOTH_AUDIO_SESSION_CONTROL_H_
