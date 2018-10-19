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
BluetoothAudioProvider BluetoothAudioProvidersFactory::mProviderCacheA2dpOffload;
BluetoothAudioProvider BluetoothAudioProvidersFactory::mProviderCacheHearingAidMedia;
BluetoothAudioProvider BluetoothAudioProvidersFactory::mProviderCacheHearingAidVoice;

Return<void> BluetoothAudioProvidersFactory::openProvider(const SessionType sessionType,
                                                          const CodecConfiguration& codecConfig,
                                                          openProvider_cb _hidl_cb) {
  IBluetoothAudioProvider* provider;
  BluetoothAudioStatus result;

  result = BluetoothAudioStatus::SUCCESS;
  ALOGE("%s - SessionType=0x%02hhx, CodecConfig={Codec=0x%08x, MTU=0x%04x}", __func__,
         sessionType, codecConfig.encodedDataConfiguration.codecType, codecConfig.encodedDataConfiguration.peerMtu);

  provider = getProvider(sessionType);
  if (provider == nullptr) {
    result = BluetoothAudioStatus::FAILURE;
    _hidl_cb(result, provider);
    return Void();
  }

  _hidl_cb(result, provider);
  return Void();
}

BluetoothAudioProvider* BluetoothAudioProvidersFactory::getProvider(const SessionType sessionType) {
  switch (sessionType) {
    case SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH:
      return &mProviderCacheA2dpLegacy;
      break;

    case SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH:
      return &mProviderCacheA2dpOffload;
      break;

    case SessionType::HEARING_AID_SOFTWARE_ENCODING_MEDIA_PATH:
      return &mProviderCacheHearingAidMedia;
      break;

    case SessionType::HEARING_AID_SOFTWARE_ENCODING_VOICE_PATH:
      return &mProviderCacheHearingAidVoice;
      break;

    default:
      return nullptr;
  }
}

IBluetoothAudioProvidersFactory* HIDL_FETCH_IBluetoothAudioProvidersFactory(const char* /* name */) {
  return new BluetoothAudioProvidersFactory();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

