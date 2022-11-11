/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.tv.earc;

import android.hardware.tv.earc.IEArcCallback;
import android.hardware.tv.earc.IEArcStatus;

/**
 * eARC HAL interface definition
 */
@VintfStability
interface IEArc {
    /**
     * Function to enable or disable eARC on the device.
     */
    void setEArcEnabled(in boolean enabled);

    /**
     * Function to check if eARC is enabled on the device.
     */
    boolean isEArcEnabled();

    /**
     * Function to set callback that the HAL will use to notify the system of connection state
     * changes and capabilities of connected devices.
     *
     * @param callback The callback object to pass the events to the system. A previously registered
     *        callback should be replaced by this new object. If callback is {@code null} the
     *        previously registered callback should be deregistered.
     */
    void setCallback(in IEArcCallback callback);

    /**
     * Getter for the current eARC state of a port.
     *
     * @param portId The port ID for which the state is to be reported.
     * @return The state of the port.
     */
    IEArcStatus getState(in int portId);

    /**
     * Getter for the most recent capabilities reported by the device connected to port.
     *
     * @param portId The port ID on which the device is connected.
     * @return The raw, unparsed audio capabilities
     */
    byte[] getLastReportedAudioCapabilities(in int portId);
}
