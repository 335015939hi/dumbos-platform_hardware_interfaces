/*
 * Copyright (C) 2023 The Android Open Source Project
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
#include <vector>

#include "RemoteSubmixUtils.h"

namespace aidl::android::hardware::audio::core::r_submix {

namespace {

const std::vector<AudioChannelLayout> kSupportedChannelMask = {
        AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                AudioChannelLayout::LAYOUT_MONO),
        AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                AudioChannelLayout::LAYOUT_STEREO)};

const std::vector<int> kSupportedSampleRates = {8000,  11025, 12000, 16000, 22050,
                                                24000, 32000, 44100, 48000};

}  // namespace

bool isChannelMaskSupported(const AudioChannelLayout& channelMask) {
    if (std::find(kSupportedChannelMask.begin(), kSupportedChannelMask.end(), channelMask) !=
        kSupportedChannelMask.end()) {
        return true;
    }
    return false;
}

bool isSampleRateSupported(int sampleRate) {
    if (std::find(kSupportedSampleRates.begin(), kSupportedSampleRates.end(), sampleRate) !=
        kSupportedSampleRates.end()) {
        return true;
    }
    return false;
}

}  // namespace aidl::android::hardware::audio::core::r_submix
