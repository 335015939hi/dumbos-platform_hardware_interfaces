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

#ifndef ANDROID_HARDWARE_BLUETOOTH_AUDIO_V2_0_BluetoothAudioProvidersFactory_H_
#define ANDROID_HARDWARE_BLUETOOTH_AUDIO_V2_0_BluetoothAudioProvidersFactory_H_

#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvidersFactory.h>

#include "BluetoothAudioProvider.h"
#include "A2dpLegacyAudioProvider.h"
#include "A2dpOffloadAudioProvider.h"
#include "HearingAidAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

class BluetoothAudioProvidersFactory : public IBluetoothAudioProvidersFactory {
  public:
    BluetoothAudioProvidersFactory() {}

    Return<void> openProvider(const SessionType sessionType,
                              openProvider_cb _hidl_cb) override;

    static BluetoothAudioProvider* getProvider(const SessionType& sessionType);

  private:
    // TODO: using hash_map <SessionType, BluetoothAudioProvider>
    static A2dpLegacyAudioProvider mProviderCacheA2dpLegacy;
    static A2dpOffloadAudioProvider mProviderCacheA2dpOffload;
    static HearingAidAudioProvider mProviderCacheHearingAids;
};

extern "C" IBluetoothAudioProvidersFactory* HIDL_FETCH_IBluetoothAudioProvidersFactory(const char* name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BLUETOOTH_AUDIO_V2_0_BluetoothAudioProvidersFactory_H_
