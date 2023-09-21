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
 * Note: Audio Channel Allocation is not exposed as it is managed by the
 *       Bluetooth stack.
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

    parcelable OctetsPerCodecFrame {
        int value;
    }

    parcelable CodecFrameBlocksPerSDU {
        int value;
    }

    CodecFrameBlocksPerSDU codecFrameBlocksPerSDU;
    SamplingFrequency samplingFrequency;
    FrameDuration frameDuration;
    OctetsPerCodecFrame octetsPerCodecFrame;
}
