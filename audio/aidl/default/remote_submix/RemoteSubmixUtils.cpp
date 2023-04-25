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

#include <map>
#include <set>

#include <Utils.h>

#include "core-impl/utils.h"

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::media::audio::common::AudioChannelLayout;

namespace aidl::android::hardware::audio::core::r_submix {

namespace {

static constexpr AUDIO_CHANNEL_LAYOUT_MONO = DEFINE_CHANNEL_LAYOUT_MASK(MONO);
static constexpr AUDIO_CHANNEL_LAYOUT_STEREO = DEFINE_CHANNEL_LAYOUT_MASK(STEREO);
static const unsigned int INVALID_CHANNEL_COUNT = 0;

static constexpr std::map<AudioChannelLayout, unsigned int> kSupportedChannelLayoutMap = {
        {AUDIO_CHANNEL_LAYOUT_MONO, getChannelCount(AUDIO_CHANNEL_LAYOUT_MONO)},
        {AUDIO_CHANNEL_LAYOUT_STEREO, getChannelCount(AUDIO_CHANNEL_LAYOUT_STEREO)}};

}  // namespace

unsigned int getChannelCountFromChannelMask(const AudioChannelLayout& channelMask) {
    switch (channelMask.getTag()) {
        case AudioChannelLayout::Tag::layoutMask: {
            return findKeyOrDefault(kSupportedChannelLayoutMap, channelMask,
                                    INVALID_CHANNEL_COUNT /*defaultValue*/);
        }
        case AudioChannelLayout::Tag::indexMask:
        case AudioChannelLayout::Tag::none:
        case AudioChannelLayout::Tag::invalid:
        case AudioChannelLayout::Tag::voiceMask:
        default:
            return 0;
    }
}

}  // namespace aidl::android::hardware::audio::core::r_submix
