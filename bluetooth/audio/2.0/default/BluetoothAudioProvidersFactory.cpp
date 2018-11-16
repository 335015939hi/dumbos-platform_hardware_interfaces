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

#define LOG_TAG "BTAudioProvidersFactory"

#include <log/log.h>

#include "BluetoothAudioProvidersFactory.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using ::android::hardware::Void;

typedef ::android::hardware::bluetooth::audio::V2_0::Status BluetoothAudioStatus;

A2dpLegacyAudioProvider BluetoothAudioProvidersFactory::mProviderCacheA2dpLegacy;
A2dpOffloadAudioProvider BluetoothAudioProvidersFactory::mProviderCacheA2dpOffload;
HearingAidAudioProvider BluetoothAudioProvidersFactory::mProviderCacheHearingAids;

Return<void> BluetoothAudioProvidersFactory::openProvider(
    const SessionType sessionType, openProvider_cb _hidl_cb) {
  BluetoothAudioProvider* provider;
  BluetoothAudioStatus result;

  result = BluetoothAudioStatus::SUCCESS;
  ALOGI("%s - SessionType=%s(0x%02hhx)", __func__,
        toString(sessionType).c_str(), sessionType);

  provider = getProvider(sessionType);
  if (provider == nullptr) {
    result = BluetoothAudioStatus::FAILURE;
  } else if (!provider->build(sessionType)) {
    result = BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION;
    provider = nullptr;
  }

  _hidl_cb(result, provider);
  return Void();
}

BluetoothAudioProvider* BluetoothAudioProvidersFactory::getProvider(
    const SessionType &sessionType) {
  switch (sessionType) {
    case SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH:
      return &mProviderCacheA2dpLegacy;
      break;
    case SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH:
      return &mProviderCacheA2dpOffload;
      break;
    case SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH:
      return &mProviderCacheHearingAids;
      break;
    default:
      return nullptr;
  }
}

IBluetoothAudioProvidersFactory* HIDL_FETCH_IBluetoothAudioProvidersFactory(
    const char* /* name */) {
  return new BluetoothAudioProvidersFactory();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

