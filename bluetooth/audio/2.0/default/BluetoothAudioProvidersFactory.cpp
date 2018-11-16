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

#define LOG_TAG "BTAudioProvidersFactory"

#include <log/log.h>

#include "BluetoothAudioProvidersFactory.h"
#include "OffloadSupportedCodecsDB.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using ::android::hardware::hidl_vec;
using ::android::hardware::Void;

using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;

A2dpSoftwareAudioProvider
    BluetoothAudioProvidersFactory::a2dp_software_provider_instance_;
A2dpOffloadAudioProvider
    BluetoothAudioProvidersFactory::a2dp_offload_provider_instance_;
HearingAidAudioProvider
    BluetoothAudioProvidersFactory::hearing_aid_provider_instance_;

Return<void> BluetoothAudioProvidersFactory::openProvider(
    const SessionType sessionType, openProvider_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  BluetoothAudioStatus status = BluetoothAudioStatus::SUCCESS;
  BluetoothAudioProvider* provider = nullptr;
  switch (sessionType) {
    case SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH:
      provider = &a2dp_software_provider_instance_;
      break;
    case SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH:
      provider = &a2dp_offload_provider_instance_;
      break;
    case SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH:
      provider = &hearing_aid_provider_instance_;
      break;
    default:
      status = BluetoothAudioStatus::FAILURE;
  }
  if (provider && !provider->isValid(sessionType)) {
    provider = nullptr;
    status = BluetoothAudioStatus::FAILURE;
    LOG(ERROR) << __func__ << " - SessionType=" << toString(sessionType)
               << ", status=" << toString(status);
  }
  _hidl_cb(status, provider);
  return Void();
}

Return<void> BluetoothAudioProvidersFactory::getOffloadCodecCapabilities(
    const SessionType sessionType, getOffloadCodecCapabilities_cb _hidl_cb) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(sessionType);
  hidl_vec<CodecCapability> codec_caps = hidl_vec<CodecCapability>(0);
  if (sessionType != SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
    _hidl_cb(BluetoothAudioStatus::FAILURE, codec_caps);
    return Void();
  }
  _hidl_cb(BluetoothAudioStatus::SUCCESS, codec_caps);
  return Void();
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
