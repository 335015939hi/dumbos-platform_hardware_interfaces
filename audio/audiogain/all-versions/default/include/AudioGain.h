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

#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace audio {
namespace audiogain {
namespace CPP_VERSION {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGain;
using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGainCallback;
using ::android::hardware::audio::audiogain::CPP_VERSION::Result;

struct AudioGain : public IAudioGain {
public:
    // Methods from IAudioGain follow.
    Return<Result> registerAudioGainCallback(const sp<IAudioGainCallback> &callback) override;
    Return<Result> unregisterAudioGainCallback(const sp<IAudioGainCallback> &callback) override;

    // Implementation details
    AudioGain() = default;
};

// this is only for passthrough implementations
extern "C" IAudioGain* HIDL_FETCH_IAudioGain(const char* name);

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audiogain
}  // namespace audio
}  // namespace hardware
}  // namespace android
