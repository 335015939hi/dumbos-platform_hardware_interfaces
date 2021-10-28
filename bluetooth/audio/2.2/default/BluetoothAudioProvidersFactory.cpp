/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "BTAudioProvidersFactory"

#include "BluetoothAudioProvidersFactory.h"

#include <android-base/logging.h>

#include "BluetoothAudioSupportedCodecsDB_2_1.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_2 {
namespace implementation {

using ::android::hardware::hidl_vec;
using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::CodecCapabilities;

using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;

Return<void> BluetoothAudioProvidersFactory::openProvider(
    const V2_0::SessionType sessionType, openProvider_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  BluetoothAudioStatus status = BluetoothAudioStatus::SUCCESS;

  _hidl_cb(status, nullptr);
  return Void();
}

Return<void> BluetoothAudioProvidersFactory::openProvider_2_1(
    const V2_1::SessionType sessionType, openProvider_2_1_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  BluetoothAudioStatus status = BluetoothAudioStatus::SUCCESS;

  _hidl_cb(status, nullptr);
  return Void();
}

Return<void> BluetoothAudioProvidersFactory::getProviderCapabilities(
    const V2_0::SessionType sessionType, getProviderCapabilities_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  hidl_vec<V2_0::AudioCapabilities> audio_capabilities =
      hidl_vec<V2_0::AudioCapabilities>(0);

  _hidl_cb(audio_capabilities);
  return Void();
}

Return<void> BluetoothAudioProvidersFactory::getProviderCapabilities_2_1(
    const V2_1::SessionType sessionType,
    getProviderCapabilities_2_1_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  hidl_vec<V2_1::AudioCapabilities> audio_capabilities =
      hidl_vec<V2_1::AudioCapabilities>(0);
  _hidl_cb(audio_capabilities);
  return Void();
}

IBluetoothAudioProvidersFactory* HIDL_FETCH_IBluetoothAudioProvidersFactory(
    const char* /* name */) {
  return new BluetoothAudioProvidersFactory();
}

}  // namespace implementation
}  // namespace V2_2
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
