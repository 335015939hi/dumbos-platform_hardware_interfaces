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

#define LOG_TAG "BTAudioProviderA2dpLegacy"

#include <log/log.h>

#include "A2dpLegacyAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::hidl_death_recipient;
using android::hardware::Void;

Return<void> A2dpLegacyAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const SessionType sessionType,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  BluetoothAudioStatus result;

  result = BluetoothAudioStatus::SUCCESS;
  ALOGI("%s - SessionType=0x%02hhx, CodecConfig={Codec=0x%08x, MTU=0x%04x}", __func__,
         sessionType, codecConfig.encodedDataConfiguration.codecType, codecConfig.encodedDataConfiguration.peerMtu);
  // TODO: should we check whether the codec is supported or not?

  death_recipient_->setHasDied(false);
  hostIf->linkToDeath(death_recipient_, 0);

  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */

  has_session_ = true;
  this->sessionType = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
  stack_iface_ = hostIf;
  a2dpCodecConfig = codecConfig;

  unlink_cb_ = [hostIf](sp<BluetoothAudioDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      ALOGI("Skipping unlink call, service died.");
    else
      hostIf->unlinkToDeath(death_recipient);
  };

  _hidl_cb(result, *mDataMQ->getDesc());
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
