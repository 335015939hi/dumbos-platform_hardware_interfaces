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

import android.hardware.bluetooth.ranging.BluetoothChannelSoundingParameters;
import android.hardware.bluetooth.ranging.IBluetoothChannelSoundingSession;
import android.hardware.bluetooth.ranging.IBluetoothChannelSoundingSessionCallback;
import android.hardware.bluetooth.ranging.SessionType;
import android.hardware.bluetooth.ranging.VendorSpecificData;

/**
 * The interface for the Bluetooth stack to open session for channel sounding
 */
@VintfStability
interface IBluetoothChannelSounding {
    /**
     * API to get vendor specific data
     *
     * @return an array of vendor specifc data
     */
    VendorSpecificData[] getVendorSpecificData();

    /**
     * API to get supported session types of the HAL
     *
     * @return an array of supported session types
     */
    SessionType[] getSupportedSessionTypes();
    IBluetoothChannelSoundingSession openSession(in BluetoothChannelSoundingParameters params,
            IBluetoothChannelSoundingSessionCallback callback);
}
