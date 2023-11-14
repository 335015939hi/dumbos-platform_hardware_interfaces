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

import android.hardware.bluetooth.ranging.DeviceAddress;
import android.hardware.bluetooth.ranging.Role;
import android.hardware.bluetooth.ranging.SessionType;
import android.hardware.bluetooth.ranging.VendorSpecificData;

@VintfStability
parcelable BluetoothChannelSoundingParameters {
    SessionType sessionType;
    // Acl handle of the connection
    char aclHandle;
    // Needed in case of EATT which may use dynamic channel for GATT
    char l2capCid;
    // ATT handle of the RAS Real-time Procedure Data
    char realTimeProcedureDataAttHandle;
    Role role;
    // If sounding phase-based ranging is supported by the local device
    boolean localSupportSoundingPhaseBasedRanging;
    // If sounding phase-based ranging is supported by the remote device
    boolean remoteSupportSoundingPhaseBaseRanging;
    DeviceAddress address;
    VendorSpecificData[] vendorSpecificData;
    // Bit mask of preferred algortihm type
    // bit 0 : indoor
    // bit 1 : line of sight
    // bit 2 : enhance security (NADM)
    int preferredAlgortihmType;
}
