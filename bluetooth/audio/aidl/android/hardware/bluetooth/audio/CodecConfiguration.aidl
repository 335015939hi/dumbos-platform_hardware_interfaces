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

import android.hardware.bluetooth.audio.AacParameters;
import android.hardware.bluetooth.audio.AptxParameters;
import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.LdacParameters;
import android.hardware.bluetooth.audio.SbcParameters;

/**
 * Used to configure a Hardware Encoding session.
 * AptX and AptX-HD both use the AptxParameters field.
 */
@VintfStability
parcelable CodecConfiguration {
    @VintfStability
    union CodecSpecific {
        SbcParameters sbcConfig;
        AacParameters aacConfig;
        LdacParameters ldacConfig;
        AptxParameters aptxConfig;
    }
    CodecType codecType;
    /**
     * The encoded audio bitrate in bits / second.
     * 0x00000000 - The audio bitrate is not specified / unused
     * 0x00000001 - 0x00FFFFFF - Encoded audio bitrate in bits/second
     * 0x01000000 - 0xFFFFFFFF - Reserved
     *
     * The HAL needs to support all legal bitrates for the selected codec.
     */
    int encodedAudioBitrate;
    /**
     * Peer MTU (in two-octets)
     */
    int peerMtu;
    /**
     * Content protection by SCMS-T
     */
    boolean isScmstEnabled;
    CodecSpecific config;
}
