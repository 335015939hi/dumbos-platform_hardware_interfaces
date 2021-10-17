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

import android.hardware.bluetooth.audio.BitsPerSample;
import android.hardware.bluetooth.audio.SampleRate;
import android.hardware.bluetooth.audio.SbcAllocMethod;
import android.hardware.bluetooth.audio.SbcBlockLength;
import android.hardware.bluetooth.audio.SbcChannelMode;
import android.hardware.bluetooth.audio.SbcNumSubbands;

/**
 * Used for Hardware Encoding SBC codec parameters.
 * minBitpool and maxBitpool are not bitfields.
 */
@VintfStability
parcelable SbcParameters {
    SampleRate sampleRate;
    SbcChannelMode channelMode;
    SbcBlockLength blockLength;
    SbcNumSubbands numSubbands;
    SbcAllocMethod allocMethod;
    BitsPerSample bitsPerSample;
    /*
     * range from 2 to 250.
     */
    int minBitpool;
    /*
     * range from 2 to 250.
     */
    int maxBitpool;
}
