/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "bt_audio_lib.h"

#include "BluetoothAudioProvidersFactory.h"
#include "BluetoothAudioProvider.h"

using ::android::hardware::bluetooth::audio::V2_0::implementation::BluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::implementation::BluetoothAudioProvider;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;

const sp<IBluetoothAudioPort> bt_audio_get_port_ctrl_path(
    const SessionType &sessionType) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    return provider->getAssociatedPortCtrl();
  else
    return nullptr;
}

const DataMQ::Descriptor* bt_audio_get_data_fmq(
    const SessionType &sessionType) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    return provider->getStreamDataFMQ();
  }
  return nullptr;
}

const PcmDataConfiguration& bt_audio_get_pcm_data_cfg(
    const SessionType &sessionType) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    return provider->getStreamPcmDataConfig();
  }
  return BluetoothAudioProvider::kInvalidPcmConfiguration;
}

bool bt_audio_get_presentation_position(const SessionType &sessionType,
                                        timespec &remote_delay_report,
                                        uint64_t &total_bytes_readed,
                                        timespec &data_position) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    return provider->getStreamPresentationPosition(remote_delay_report,
                                                   total_bytes_readed,
                                                   data_position);
  }
  return false;
}

void bt_audio_update_tracks_metadata(
    const SessionType &sessionType,
    const struct source_metadata *source_metadata) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    provider->updateStreamTracksMetadata(source_metadata);
  }
}

uint16_t bt_audio_set_port_ctrl_result_cb(
    const SessionType &sessionType,
    std::function<void(const uint16_t&,
                       const BluetoothAudioStatus&)> &ctrl_res_cb,
    std::function<void(const uint16_t&)> &session_changed_cb) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr && ctrl_res_cb != nullptr &&
      session_changed_cb != nullptr)
    return provider->registerControlResultCback(ctrl_res_cb, session_changed_cb);
  return OBSERVERS_CTRL_KEY_UNDEF;
}

void bt_audio_reset_port_ctrl_result_cb(const SessionType &sessionType,
                                        uint16_t &ctrl_key) {
  BluetoothAudioProvider *provider =
      BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    provider->unregisterControlResultCback(ctrl_key);
}
