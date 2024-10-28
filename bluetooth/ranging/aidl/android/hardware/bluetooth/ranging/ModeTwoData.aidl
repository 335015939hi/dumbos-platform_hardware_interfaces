/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.ComplexNumber;

/**
 * Raw ranging data of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable ModeTwoData {
    /**
     * Antenna Permutation Index for the chosen Num_Antenna_Paths parameter used during the
     * phase measurement stage of the CS step
     */
    byte antennaPermutationIndex;
    /**
     * Phase Correction Term for (Num_Antenna_Paths + 1) CS tone
     * ComplexNumber#real - indicates the I sample
     * ComplexNumber#imaginary - indicates the Q sample
     */
    ComplexNumber[] tonePctIQSample;
    /**
     * Tone quality indicator for (Num_Antenna_Paths + 1) CS tone
     * bits 0 to 3:
     * ** 0x0 = Tone quality is high
     * ** 0x1 = Tone quality is medium
     * ** 0x2 = Tone quality is low
     * ** 0x3 = Tone quality is unavailable
     * bits 4 to 7:
     * ** 0x0 = Not tone extension slot
     * ** 0x1 = Tone extension slot; tone not expected to be present
     * ** 0x2 = Tone extension slot; tone expected to be present
     */
    byte[] toneQualityIndicator;
}
