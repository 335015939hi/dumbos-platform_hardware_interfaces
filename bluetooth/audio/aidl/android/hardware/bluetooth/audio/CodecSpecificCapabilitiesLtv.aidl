/*
 * Copyright 2021 The Android Open Source Project
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

@VintfStability
union CodecSpecificCapabilitiesLtv {
    parcelable SupportedSamplingFrequencies {
        boolean b8000Hz;
        boolean b11025Hz;
        boolean b16000Hz;
        boolean b22050Hz;
        boolean b24000Hz;
        boolean b32000Hz;
        boolean b44100Hz;
        boolean b48000Hz;
        boolean b88200Hz;
        boolean b96000Hz;
        boolean b176400Hz;
        boolean b192000Hz;
        boolean b384000Hz;
    }

    parcelable SupportedFrameDurations {
        boolean b7ms5;
        boolean b10ms;
        boolean b7ms5Preferred;
        boolean b10msPreferred;
    }

    parcelable SupportedAudioChannelCounts {
        boolean b1;
        boolean b2;
        boolean b3;
        boolean b4;
        boolean b5;
        boolean b6;
        boolean b7;
        boolean b8;
    }

    parcelable SupportedOctetsPerCodecFrame {
        int minimum;
        int maximum;
    }

    parcelable SupportedMaxCodecFramesPerSDU {
        int value;
    }

    SupportedSamplingFrequencies supportedSamplingFrequencies;
    SupportedFrameDurations supportedFrameDurations;
    SupportedAudioChannelCounts supportedAudioChannelCounts;
    SupportedOctetsPerCodecFrame supportedOctetsPerCodecFrame;
    SupportedMaxCodecFramesPerSDU supportedMaxCodecFramesPerSDU;
}
