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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.ChannelSoundingSingleSideData;
import android.hardware.bluetooth.ranging.ModeType;

/**
 * Raw ranging data of Channel Sounding
 */
@VintfStability
parcelable ChannelSoudingRawData {
    // Common data for both initator and reflector sided
    ChannelSoundingSingleSideData initiatorData;
    ChannelSoundingSingleSideData reflectorData;

    // The channel indices of every step in a CS procedure (in time order)
    byte[] stepChannels;

    // Toa_tod_initator from mode-1 or mode-3 steps in a CS procedures (in time order)
    // Time of flight = 0.5 * (toa_tod_initiator - tod_toa_reflector)
    char[] toaTodInitiator;

    // Tod_toa_reflector from mode-1 or mode-3 steps in a CS procedures (in time order)
    // Time of flight = 0.5 * (toa_tod_initiator - tod_toa_reflector)
    char[] todToaReflector;

    // CS mode (0, 1, 2, 3) of each CS step
    ModeType[] stepMode;

    // Number of antenna paths (1 to 4) reported in the procedure
    byte numAntennaPaths;

    // Timestamp when the procedure is created. Using epoch time in ms (e.g., 1697673127175)
    long timestamp;
}
