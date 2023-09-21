/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

/**
 * Used to exchange generic remote device configuration between the stack and
 * the provider. As defined in Bluetooth Assigned Numbers, Sec. 6.12.5.
 */
@VintfStability
union CodecSpecificConfigurationLtv {
    @Backing(type="byte")
    enum SamplingFrequency {
        HZ8000 = 0x01,
        HZ11025 = 0x02,
        HZ16000 = 0x03,
        HZ22050 = 0x04,
        HZ24000 = 0x05,
        HZ32000 = 0x06,
        HZ44100 = 0x07,
        HZ48000 = 0x08,
        HZ88200 = 0x09,
        HZ96000 = 0x0A,
        HZ176400 = 0x0B,
        HZ192000 = 0x0C,
        HZ384000 = 0x0D,
    }

    @Backing(type="byte")
    enum FrameDuration {
        US7500 = 0x00,
        US10000 = 0x01,
    }

    @Backing(type="int")
    enum AudioChannelAllocation {
        NOT_ALLOWED = 0x00000000,
        FRONT_LEFT = 0x00000001,
        FRONT_RIGHT = 0x00000002,
        FRONT_CENTER = 0x00000004,
        LOW_FREQUENCY_EFFECTS_1 = 0x00000008,
        BACK_LEFT = 0x00000010,
        BACK_RIGHT = 0x00000020,
        FRONT_LEFT_OF_CENTER = 0x00000040,
        FRONT_RIGHT_OF_CENTER = 0x00000080,
        BACK_CENTER = 0x00000100,
        LOW_FREQUENCY_EFFECTS_2 = 0x00000200,
        SIDE_LEFT = 0x00000400,
        SIDE_RIGHT = 0x00000800,
        TOP_FRONT_LEFT = 0x00001000,
        TOP_FRONT_RIGHT = 0x00002000,
        TOP_FRONT_CENTER = 0x00004000,
        TOP_CENTER = 0x00008000,
        TOP_BACK_LEFT = 0x00010000,
        TOP_BACK_RIGHT = 0x00020000,
        TOP_SIDE_LEFT = 0x00040000,
        TOP_SIDE_RIGHT = 0x00080000,
        TOP_BACK_CENTER = 0x00100000,
        BOTTOM_FRONT_CENTER = 0x00200000,
        BOTTOM_FRONT_LEFT = 0x00400000,
        BOTTOM_FRONT_RIGHT = 0x00800000,
        FRONT_LEFT_WIDE = 0x01000000,
        FRONT_RIGHT_WIDE = 0x02000000,
        LEFT_SURROUND = 0x04000000,
        RIGHT_SURROUND = 0x08000000,
    }

    parcelable OctetsPerCodecFrame {
        int value;
    }

    parcelable CodecFrameBlocksPerSDU {
        int value;
    }

    CodecFrameBlocksPerSDU codecFrameBlocksPerSDU;
    SamplingFrequency samplingFrequency;
    FrameDuration frameDuration;
    AudioChannelAllocation[] audioChannelAllocation;
    OctetsPerCodecFrame octetsPerCodecFrame;
}
