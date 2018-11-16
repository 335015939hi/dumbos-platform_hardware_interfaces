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

#pragma once

#include <mutex>
#include <unordered_map>

#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>
#include <fmq/MessageQueue.h>
#include <hardware/audio.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace bluetooth {
namespace audio {

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

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
static constexpr uint16_t kObserversCookieSize = 0x0010;  // 0x0000 ~ 0x000f
constexpr uint16_t kObserversCookieUndefined =
    (static_cast<uint16_t>(SessionType::UNKNOWN) << 8 & 0xff00);
inline SessionType ObserversCookieGetSessionType(uint16_t cookie) {
  return static_cast<SessionType>(cookie >> 8 & 0x00ff);
}
inline uint16_t ObserversCookieGetInitValue(SessionType session_type) {
  return (static_cast<uint16_t>(session_type) << 8 & 0xff00);
}
inline uint16_t ObserversCookieGetUpperBound(SessionType session_type) {
  return (static_cast<uint16_t>(session_type) << 8 & 0xff00) +
         kObserversCookieSize;
}

// Those APIs for the bluetooth_audio module to register callbacks of started /
// suspended and session changed
struct PortStatusCallbacks {
  // control_result_cb_ - when the Bluetooth stack reports results of
  // streamStarted and streamSuspended, the BluetoothAudioProvider will invoke
  // this callback to report to the bluetooth_audio module.
  // @param: cookie - indicates which output of the bluetooth_audio should
  //         handle it
  // @param: start_resp - this report is for startStream or not
  // @param: status - the result of startStream
  std::function<void(uint16_t cookie, bool start_resp,
                     const BluetoothAudioStatus& status)>
      control_result_cb_;
  // session_changed_cb_ - when the Bluetooth stack start / end session, the
  // BluetoothAudioProvider will invoke this callback to notify to the
  // bluetooth_audio module.
  // @param: cookie - indicates which output of the bluetooth_audio should
  //         handle it
  std::function<void(uint16_t cookie)> session_changed_cb_;
};

class BluetoothAudioSession {
 public:
  BluetoothAudioSession(const SessionType& session_type);

  // The common API helps to check if this session is ready or not
  // @return: true if the Bluetooth stack has started the specified session
  bool IsSessionReady();

  // The API reports the Bluetooth stack has started the session, and will
  // invoke session_changed_cb_ to notify related outputs of the bluetooth_audio
  void OnSessionStarted(const sp<IBluetoothAudioPort> stack_iface,
                        const DataMQ::Descriptor* dataMQ,
                        const CodecConfiguration& codec_config);

  // The API reports the Bluetooth stack has ended the session, and will invoke
  // session_changed_cb_ to notify related outputs of the bluetooth_audio
  void OnSessionEnded();

  // The API reports the Bluetooth stack has notified the result of startStream
  // and suspendStream, and will invoke control_result_cb_ to notify related
  // outputs of the bluetooth_audio
  void ReportControlStatus(bool start_resp, const BluetoothAudioStatus& status);

  // The control API helps the bluetooth_audio module to register
  // PortStatusCallbacks
  // @return: cookie - the assigned number to this output of the
  //          bluetooth_audio
  uint16_t RegisterStatusCback(const PortStatusCallbacks& cbacks);

  // The control API helps the bluetooth_audio module to unregister
  // PortStatusCallbacks
  // @param: cookie - indicates which output of the bluetooth_audio is
  void UnregisterStatusCback(uint16_t cookie);

  // The control API for the bluetooth_audio module to get the current
  // CodecConfiguration
  const CodecConfiguration& GetCodecConfig();

  // Those control APIs for the bluetooth_audio module to start / suspend / stop
  // stream, to check position, and to update metadata.
  bool StartStream();
  bool SuspendStream();
  void StopStream();
  bool GetPresentationPosition(uint64_t* remote_delay_report_ns,
                               uint64_t* total_bytes_readed,
                               timespec* data_position);
  void UpdateTracksMetadata(const struct source_metadata* source_metadata);

  // The control API writes stream to FMQ
  size_t OutWritePcmData(const void* buffer, size_t bytes);

  static const CodecConfiguration kInvalidCodecConfiguration;
  static const PcmDataConfiguration& kInvalidPcmDataConfiguration;

 private:
  // using recursive_mutex to allow hwbinder to re-enter agian.
  std::recursive_mutex mutex_;
  SessionType session_type_;

  // audio control path to use for both of software and offloading
  sp<IBluetoothAudioPort> stack_iface_;
  // audio data path (FMQ) for software encoding
  std::unique_ptr<DataMQ> mDataMQ;
  // audio data configuration for both of software and offloading
  CodecConfiguration codec_config_;

  // saving those callbacks of the bluetooth_audio module registered
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
  // private and internal function invokes the registered session_changed_cb_
  void ReportSessionStatus();
};

class BluetoothAudioSessionInstance {
 public:
  // The API to fetch the session of A2DP / Hearing Aid
  static std::shared_ptr<BluetoothAudioSession> GetSessionInstance(
      const SessionType& session_type);

 private:
  static std::unique_ptr<BluetoothAudioSessionInstance> instance_ptr;
  std::mutex mutex_;
  std::unordered_map<SessionType, std::shared_ptr<BluetoothAudioSession>>
      sessions_map_;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
