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

import android.hardware.bluetooth.ranging.ChannelSoudingRawData;
import android.hardware.bluetooth.ranging.Config;
import android.hardware.bluetooth.ranging.Filter;
import android.hardware.bluetooth.ranging.PctFormat;
import android.hardware.bluetooth.ranging.Reason;
import android.hardware.bluetooth.ranging.VendorSpecificData;

/**
 * Session of Channel Sounding get from IBluetoothChannelSounding.openSession().
 */
@VintfStability
interface IBluetoothChannelSoundingSession {
    Filter getFilter();
    PctFormat getPctFormat();
    Config getPreferedCofig();
    // HAL needs to tell the BT stack
    VendorSpecificData[] getVendorSpecificReplies();

    void writeRawData(in ChannelSoudingRawData rawData);
    // Close the current session. Object is no longer useful after this method.
    void close(Reason reason);
}
