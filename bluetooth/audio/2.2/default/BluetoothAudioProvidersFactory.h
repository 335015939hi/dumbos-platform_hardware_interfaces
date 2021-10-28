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

#pragma once

#include <android/hardware/bluetooth/audio/2.2/IBluetoothAudioProvidersFactory.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_2 {
namespace implementation {

class BluetoothAudioProvidersFactory : public IBluetoothAudioProvidersFactory {
 public:
  BluetoothAudioProvidersFactory() {}

  Return<void> openProvider(const V2_0::SessionType sessionType,
                            openProvider_cb _hidl_cb) override;

  Return<void> getProviderCapabilities(
      const V2_0::SessionType sessionType,
      getProviderCapabilities_cb _hidl_cb) override;

  Return<void> openProvider_2_1(const V2_1::SessionType sessionType,
                                openProvider_2_1_cb _hidl_cb) override;

  Return<void> getProviderCapabilities_2_1(
      const V2_1::SessionType sessionType,
      getProviderCapabilities_2_1_cb _hidl_cb) override;
};

extern "C" IBluetoothAudioProvidersFactory*
HIDL_FETCH_IBluetoothAudioProvidersFactory(const char* name);

}  // namespace implementation
}  // namespace V2_2
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
