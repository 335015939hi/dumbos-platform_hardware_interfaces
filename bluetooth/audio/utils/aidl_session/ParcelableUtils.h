/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/bluetooth/audio/AudioCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

class ParcelableUtils {
 public:
  static void Update(AudioConfiguration& dst, const AudioConfiguration& src) {
    switch (src.getTag()) {
      case AudioConfiguration::pcmConfig:
        dst.set<AudioConfiguration::pcmConfig>(
            src.get<AudioConfiguration::pcmConfig>());
        break;
      case AudioConfiguration::codecConfig:
        dst.set<AudioConfiguration::codecConfig>(
            src.get<AudioConfiguration::codecConfig>());
        break;
      case AudioConfiguration::leAudioConfig:
        dst.set<AudioConfiguration::leAudioConfig>(
            src.get<AudioConfiguration::leAudioConfig>());
        break;
    }
  }
  static void Update(AudioCapabilities& dst, const AudioCapabilities& src) {
    switch (src.getTag()) {
      case AudioCapabilities::pcmCapabilities:
        dst.set<AudioCapabilities::pcmCapabilities>(
            src.get<AudioCapabilities::pcmCapabilities>());
        break;
      case AudioCapabilities::codecCapabilities:
        dst.set<AudioCapabilities::codecCapabilities>(
            src.get<AudioCapabilities::codecCapabilities>());
        break;
      case AudioCapabilities::leAudioCapabilities:
        dst.set<AudioCapabilities::leAudioCapabilities>(
            src.get<AudioCapabilities::leAudioCapabilities>());
        break;
    }
  }
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl