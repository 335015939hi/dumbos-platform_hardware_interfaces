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

#include <log/log.h>

#include "BluetoothAudioProvidersFactory.h"
#include "BluetoothAudioProvider.h"

using ::android::hardware::bluetooth::audio::V2_0::implementation::BluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::implementation::BluetoothAudioProvider;

hidl_vec<uint8_t>* bt_audio_get_data_buffer(SessionType sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    return provider->getStreamDataBuffer();
  else
    return nullptr;
}

const DataMQ::Descriptor* bt_audio_get_data_fmq(SessionType sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    return provider->getStreamDataFMQ();
  }
  return nullptr;
}

const sp<IBluetoothAudioPort> bt_audio_get_port_ctrl_path(SessionType sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    return provider->getAssociatedPortCtrl();
  else
    return nullptr;
}

void bt_audio_set_port_ctrl_result_cb(const SessionType& sessionType,
                                      std::function<void(const SessionType, const BluetoothAudioStatus&)>& res_cb,
                                      std::function<void(const SessionType)>& end_cb) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr && res_cb != nullptr && end_cb != nullptr)
    provider->registerControlResultCback(res_cb, end_cb);
}

void bt_audio_reset_port_ctrl_result_cb(const SessionType& sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    provider->unregisterControlResultCback();
}
