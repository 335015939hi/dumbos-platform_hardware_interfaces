/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.modem;

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.modem.FrequencyRange;

@VintfStability
parcelable ActivityStatsTechSpecificInfo {
    /**
     * Radio access technology. It shall not be set if the Activity statistics
     * is RAT independent
     */
    AccessNetwork rat;
    /**
     * Rrequency range. It shall not be set if the Activity statistics
     * is frequency range independent
     */
    FrequencyRange frequencyRange;
    /**
     * Each index represent total time (in ms) during which the transmitter is active/awake for a
     * particular power range as shown below.
     * index 0 = tx_power < 0dBm
     * index 1 = 0dBm < tx_power < 5dBm
     * index 2 = 5dBm < tx_power < 15dBm
     * index 3 = 15dBm < tx_power < 20dBm
     * index 4 = tx_power > 20dBm
     */
    int[] txmModetimeMs;
    /**
     * Total time (in ms) for which receiver is active/awake and the transmitter is inactive
     */
    int rxModeTimeMs;
}
