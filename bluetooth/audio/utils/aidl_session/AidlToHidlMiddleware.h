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

#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/SessionType.h>

#include "../aidl_session/BluetoothAudioSession.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

class AidlToHidlMiddleware {
 public:
  static bool IsAidlAvailable();

  static bool IsSessionReady(const SessionType& session_type);

  static uint16_t RegisterControlResultCback(const SessionType& session_type,
                                             const PortStatusCallbacks& cbacks);

  static void UnregisterControlResultCback(const SessionType& session_type,
                                           uint16_t cookie);

  static const AudioConfiguration GetAudioConfig(
      const SessionType& session_type);

  static bool StartStream(const SessionType& session_type);

  static void StopStream(const SessionType& session_type);

  static bool SuspendStream(const SessionType& session_type);

  static bool GetPresentationPosition(
      const SessionType& session_type,
      PresentationPosition& presentation_position);

  static void UpdateSourceMetadata(
      const SessionType& session_type,
      const struct source_metadata* source_metadata);

  static void UpdateSinkMetadata(const SessionType& session_type,
                                 const struct sink_metadata& sink_metadata);

  static size_t OutWritePcmData(const SessionType& session_type,
                                const void* buffer, size_t bytes);

  static size_t InReadPcmData(const SessionType& session_type, void* buffer,
                              size_t bytes);
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
