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

#ifndef BLUETOOTH_AUDIO_SESSION_H_
#define BLUETOOTH_AUDIO_SESSION_H_

#include <mutex>
#include <unordered_map>

#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>
#include <fmq/MessageQueue.h>
#include <hardware/audio.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace bluetooth {
namespace audio {

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
#define OBSERVERS_SIZE_ 0x100
#define OBSERVERS_SESSION_TYPE_MASK_ 0xFF00
#define OBSERVERS_SESSION_TYPE_OFFSET_ 8
#define OBSERVERS_GET_SESSION_TYPE(x) \
  static_cast<SessionType>((x >> OBSERVERS_SESSION_TYPE_OFFSET_) & 0x00FF)
#define OBSERVERS_COOKIES_UNDEF                \
  (static_cast<uint16_t>(SessionType::UNKNOWN) \
   << OBSERVERS_SESSION_TYPE_OFFSET_)

using ::android::sp;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;

using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;
using PcmDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::PcmDataConfiguration;
using EncodedDataConfiguration = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;

// APIs for bluetooth_audio module to register callbacks of started / suspended
// and session changed
struct PortStatusCallbacks {
  // control_result_cb_ - when Bluetooth stack reports streamStarted /
  // streamSuspended, BluetoothAudioProvider will invoke this callback to report
  // to bluetooth_audio module.
  // param: cookies - indicates which output of bluetooth_audio should handle it
  // param: start_resp - this report is for startStream or not
  // param: status - the result of startStream
  std::function<void(const uint16_t&, const bool, const BluetoothAudioStatus&)>
      control_result_cb_;
  // session_changed_cb_ - when Bluetooth stack start / end session,
  // BluetoothAudioProvider will invoke this callback to notify to
  // bluetooth_audio module.
  // param: cookies - indicates which output of bluetooth_audio should handle it
  std::function<void(const uint16_t&)> session_changed_cb_;
};

class BluetoothAudioSession {
 public:
  BluetoothAudioSession(const SessionType& session_type);

  // API reports Bluetooth stack has started the session and will invoke
  // session_changed_cb_ to notify related output of bluetooth_audio
  void OnSessionStarted(const sp<IBluetoothAudioPort> stack_iface,
                        const DataMQ::Descriptor* dataMQ,
                        const CodecConfiguration& codec_config);

  // API reports Bluetooth stack has ended the session and will invoke
  // session_changed_cb_ to notify related output of bluetooth_audio
  void OnSessionEnded();

  // API reports Bluetooth stack has notified the result of startStream /
  // suspendStream and will invoke control_result_cb_ to notify related output
  // of bluetooth_audio
  void ReportControlStatus(const bool& start_resp,
                           const BluetoothAudioStatus& status);

  // Control API helps bluetooth_audio module to register PortStatusCallbacks
  // return: cookies - the assigned number to this output of bluetooth_audio
  uint16_t RegisterStatusCback(const PortStatusCallbacks& cbacks);

  // Control API helps bluetooth_audio module to unregister PortStatusCallbacks
  // param: cookies - indicates which output of bluetooth_audio is
  void UnregisterStatusCback(const uint16_t& cookies);

  // Control API for bluetooth_audio module to get current CodecConfiguration
  const CodecConfiguration& GetCodecConfig();

  // Control APIs for bluetooth_audio module to start / suspend / stop stream,
  // to check position, and to update metadata.
  bool StartStream();
  bool SuspendStream();
  void StopStream();
  bool GetPresentationPosition(uint64_t* remote_delay_report_ns,
                               uint64_t* total_bytes_readed,
                               timespec* data_position);
  void UpdateTracksMetadata(const struct source_metadata* source_metadata);

  // Control API to write stream to FMQ
  size_t OutWritePcmData(const void* buffer, size_t bytes);

  static const CodecConfiguration kInvalidCodecConfiguration;
  static const PcmDataConfiguration& kInvalidPcmDataConfiguration;

 private:
  // using recursive_mutex to allow hwbinder to re-entry agian.
  std::recursive_mutex mutex_;
  SessionType session_type_;

  // audio control path to use for both of software (legacy) and offloading
  sp<IBluetoothAudioPort> stack_iface_;
  // audio data path (FMQ) for software encoding (legacy)
  std::unique_ptr<DataMQ> mDataMQ;
  // audio data configuration for both of software (legacy) and offloading
  CodecConfiguration codec_config_;

  // storage about bluetooth_audio module registered callbacks
  std::unordered_map<uint16_t, std::shared_ptr<struct PortStatusCallbacks>>
      observers_;

  // private and internal function to setup / reset the audio control path
  bool UpdateCtrlPath(const sp<IBluetoothAudioPort> stack_iface);
  // private and internal function to setup / reset the audio data path
  bool UpdateDataPath(const DataMQ::Descriptor* dataMQ);
  // private and internal function to setup / reset pcmDataConfiguration
  bool UpdatePcmDataConfig(const PcmDataConfiguration& pcm_config);
  // private and internal function to setup / reset encodedDataConfiguration
  bool UpdateEncodedDataConfig(const EncodedDataConfiguration& encoded_config);
  // private and internal function to check if session ready
  bool IsSessionReady();
  // private and internal function invokes the registered session_changed_cb_
  void ReportSessionStatus();
};

class BluetoothAudioSessionInstance {
 public:
  // API to fetch the session of A2DP / Hearing Aid
  static std::shared_ptr<BluetoothAudioSession> GetSessionInstance(
      const SessionType& session_type);

 private:
  static std::unique_ptr<BluetoothAudioSessionInstance> instance_ptr_;
  static std::unordered_map<SessionType, std::shared_ptr<BluetoothAudioSession>>
      sessions_map_;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android

#endif  // BLUETOOTH_AUDIO_SESSION_H_
