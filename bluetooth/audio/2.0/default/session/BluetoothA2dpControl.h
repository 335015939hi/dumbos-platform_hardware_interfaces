/*
Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

using ::android::hardware::bluetooth::audio::V2_0::SessionType;
using ::android::hardware::bluetooth::audio::V2_0::SinkLatency;

class BluetoothA2dpControl {

 private:
  static BluetoothA2dpControl *A2dpControl;
  SinkLatency sinkLatency;
  bool updateLocalLatency;
  uint32_t mtu;
  uint16_t bitrate;
  std::mutex a2dpControlMutex;

  BluetoothA2dpControl() {
    updateLocalLatency = false;
    sinkLatency.remoteDeviceAudioDelay = 0;
    sinkLatency.transmittedOctets = 0;
    mtu = 0;
    bitrate = 0;
  }

 public:

  // This function instantiate A2DP Control
  // This is called when Audio Session is ready
  static void startA2DPControl() {
    if(A2dpControl == NULL) {
      A2dpControl = new BluetoothA2dpControl();
    }
  }

  // This function returns A2DP Control instance if Audio Session is ready
  // @return: A2DP control instance if Audio Session is active else returns null
  static BluetoothA2dpControl *getA2DPControl() {
    return A2dpControl;
  }

  // This function reports the sink latency shared as delay reporting
  // Local values will be updated if data is already fetched once
  // This will return null if Audio Session is not present
  bool getSinkLatency(const SessionType& session_type,
                                      uint64_t* remote_delay_report_ns,
                                      uint64_t* total_bytes_readed,
                                      timespec* data_position);

  // This function updates the latency value based on stack feedback
  // Same value will be updated to Audio when requested
  void updateSinkLatency(uint64_t remoteDeviceAudioDelay);

  // Destructor
  static void freeA2dpControl() {
    delete A2dpControl;
    A2dpControl = NULL;
  }

};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
