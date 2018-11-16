/*
 * Copyright 2018 The Android Open Source Project
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

#include <android/hardware/bluetooth/audio/2.0/types.h>

namespace android {
namespace bluetooth {
namespace audio {

using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecCapability;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;

/**
 * Default Codec Support
 * SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
 * AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
 * aptX: mSampleRate:(44100|48000), mBitsPerSample:(16), mChannelMode:(STEREO)
 * aptX HD: mSampleRate:(44100|48000), mBitsPerSample:(24),
 *          mChannelMode:(STEREO)
 * LDAC: mSampleRate:(44100|48000|88200|96000), mBitsPerSample:(16|24|32),
 *       mChannelMode:(STEREO)
 */
constexpr CodecCapability kDefaultCodecCapabilities = {
    .codecType = CodecType::UNKNOWN,
    .pcmDataCapability =
        {
            .sampleRateBitMask = static_cast<SampleRate>(
                SampleRate::RATE_44100 | SampleRate::RATE_48000),
            .bitsPerSampleBitMask = (BitsPerSample::BITS_16),
            .channelModeBitMask = static_cast<ChannelMode>(ChannelMode::MONO |
                                                           ChannelMode::STEREO),
        },
    .codec_specific_1 = 0,
    .codec_specific_2 = 0,
    .codec_specific_3 = 0,
    .codec_specific_4 = 0};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
