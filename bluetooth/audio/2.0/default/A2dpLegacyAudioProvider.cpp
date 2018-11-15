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

using ::android::hardware::bluetooth::audio::V2_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::PlaybackTrackMetadata;

Return<void> A2dpLegacyAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {

  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  mSessionType = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
  // TODO: should we check whether the codec is supported or not?
  BluetoothAudioProvider::startSession(hostIf, codecConfig, _hidl_cb);

  return Void();
}

void A2dpLegacyAudioProvider::updateStreamTracksMetadata(const struct source_metadata *source_metadata) {
  if (has_session_ && stack_iface_) {
    ALOGD("%s - Metadata is no needed for SessionType=0x%02hhx", __func__,
          mSessionType);
  } else
    ALOGW("%s - provider has NO session", __func__);

  // [SEG_BT]
  BluetoothAudioProvider::updateStreamTracksMetadata(source_metadata);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
