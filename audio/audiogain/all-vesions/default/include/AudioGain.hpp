/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include PATH(android/hardware/audio/audiogain/FILE_VERSION/IAudioGainCallback.h)
#include PATH(android/hardware/audio/audiogain/FILE_VERSION/IAudioGain.h)

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <hardware/audio.h>

namespace android {
namespace hardware {
namespace audio {
namespace audiogain {
namespace CPP_VERSION {
namespace implementation {

struct AudioGain : public ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGain {
public:
    // Methods from android::hardware::audio::audiogain::CPP_VERSION::IAudioGain follow.
    android::hardware::Return<::android::hardware::audio::audiogain::CPP_VERSION::Result> subscribe(
                const android::sp<::android::hardware::audio::audiogain::CPP_VERSION::IAudioGainCallback> &callback) override;
    android::hardware::Return<::android::hardware::audio::audiogain::CPP_VERSION::Result> unsubscribe(
            const android::sp<::android::hardware::audio::audiogain::CPP_VERSION::IAudioGainCallback> &callback) override;

    // Implementation details
    AudioGain() = default;
};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audiogain
}  // namespace audio
}  // namespace hardware
}  // namespace android
