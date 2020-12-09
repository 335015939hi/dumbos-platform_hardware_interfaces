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

#define LOG_TAG "BluetoothA2dpControl"

#include "BluetoothA2dpControl.h"
#include "BluetoothAudioSessionControl.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

namespace android {
namespace bluetooth {
namespace audio {

BluetoothA2dpControl *BluetoothA2dpControl::A2dpControl = NULL;

bool BluetoothA2dpControl::getSinkLatency(const SessionType& session_type,
                              uint64_t* remote_delay_report_ns,
                              uint64_t* total_bytes_readed,
                              timespec* data_position) {
  std::unique_lock<std::mutex> guard(a2dpControlMutex);
  bool ret;
  if(updateLocalLatency) {
    *remote_delay_report_ns = sinkLatency.remoteDeviceAudioDelay;
    return true;
  } else {
    a2dpControlMutex.unlock();
    ret = BluetoothAudioSessionControl::GetPresentationPosition (
            session_type, &sinkLatency.remoteDeviceAudioDelay,
             total_bytes_readed, data_position);
    *remote_delay_report_ns = sinkLatency.remoteDeviceAudioDelay;
    updateLocalLatency = true;
    LOG(INFO) << __func__ << " Updating fetched Latency: " << *remote_delay_report_ns;
  }
  return ret;
}

void BluetoothA2dpControl::updateSinkLatency(uint64_t remoteDeviceAudioDelay) {
  std::unique_lock<std::mutex> guard(a2dpControlMutex);
  sinkLatency.remoteDeviceAudioDelay = remoteDeviceAudioDelay;
  LOG(INFO) << __func__ << " Latency update from stack: " << remoteDeviceAudioDelay;
  updateLocalLatency = true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
