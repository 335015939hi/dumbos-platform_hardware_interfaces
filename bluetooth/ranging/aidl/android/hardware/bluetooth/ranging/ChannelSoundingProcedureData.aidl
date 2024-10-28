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

import android.hardware.bluetooth.ranging.SubeventResultData;

@VintfStability
parcelable ChannelSoundingProcedureData {
    /**
     * CS procedure count since completion of the Channel Sounding Security Start procedure
     */
    int procedureCounter;
    /**
     * The procequre sequence since completion of the Channel Sounding Procecedure Enable procedure,
     * this is not defined by spec, BT
     */
    int procedureSequence;
    /**
     * The subevent result data of initiator
     */
    SubeventResultData[] initiatorSubeventResultData;
    /**
     * The subevent result data of reflector
     */
    SubeventResultData[] relectorSubeventResultData;
    /**
     * Parameters for vendors to place vendor-specific data.
     */
    @nullable byte[] VendorSpecificData;
    /**
     * Timestamp when the procedure is created. Using epoch time in ms (e.g., 1697673127175).
     */
    long timestampMs;
}
