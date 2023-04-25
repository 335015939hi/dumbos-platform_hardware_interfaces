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

using AudioChannelCountToMaskMap = std::map<unsigned int, AudioChannelLayout>;

static const AudioChannelLayout INVALID_CHANNEL_LAYOUT =
        AudioChannelLayout::make<AudioChannelLayout::Tag::invalid>(0);

#define DEFINE_CHANNEL_LAYOUT_MASK(n) \
    AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(AudioChannelLayout::LAYOUT_##n)

static const std::set<AudioChannelLayout> SUPPORTED_CHANNEL_LAYOUTS = {
        DEFINE_CHANNEL_LAYOUT_MASK(MONO),
        DEFINE_CHANNEL_LAYOUT_MASK(STEREO),
};

static AudioChannelCountToMaskMap make_ChannelCountToMaskMap(
        const std::set<AudioChannelLayout>& channelMasks) {
    AudioChannelCountToMaskMap channelMaskToCountMap;
    for (const auto& channelMask : channelMasks) {
        channelMaskToCountMap.emplace(getChannelCount(channelMask), channelMask);
    }
    return channelMaskToCountMap;
}

const AudioChannelCountToMaskMap& getSupportedChannelLayoutMap() {
    static const AudioChannelCountToMaskMap layouts =
            make_ChannelCountToMaskMap(SUPPORTED_CHANNEL_LAYOUTS);
    return layouts;
}

}  // namespace

unsigned int getChannelCountFromChannelMask(const AudioChannelLayout& channelMask) {
    switch (channelMask.getTag()) {
        case AudioChannelLayout::Tag::layoutMask: {
            return findKeyOrDefault(getSupportedChannelLayoutMap(),
                                    (unsigned int)getChannelCount(channelMask),
                                    0u /*defaultValue*/);
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
