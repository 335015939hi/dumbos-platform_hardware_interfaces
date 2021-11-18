/*
Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.

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
