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
    return provider->getAudioStreamBuffer();
  else
    return nullptr;
}

const DataMQ::Descriptor* bt_audio_get_data_fmq(SessionType sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr) {
    return provider->getAudioStreamFMQ();
  }
  return nullptr;
}

sp<IBluetoothAudioPort> bt_audio_get_output_ctrl_path(SessionType sessionType) {
  BluetoothAudioProvider* provider = BluetoothAudioProvidersFactory::getProvider(sessionType);
  if (provider != nullptr)
    return provider->getAudioStreamCtrl();
  else
    return nullptr;
}
