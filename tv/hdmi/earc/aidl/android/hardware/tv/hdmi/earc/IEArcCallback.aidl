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

package android.hardware.tv.hdmi.earc;

import android.hardware.tv.hdmi.earc.IEArcStatus;

/**
 * eARC HAL callback methods
 */
@VintfStability
oneway interface IEArcCallback {
    /**
     * The callback function that must be called by the eARC implementation to notify the system of
     * an eARC status change.
     * @param status The new status of the port
     * @param portId The port ID for which the state change is being reported
     */
    void onStateChange(in IEArcStatus status, in int portId);

    /**
     * The callback function that must be called by the eARC implementation to notify the system of
     * the audio capabilities reported by the connected device. Should follow a state change to
     * {@code STATUS_EARC_CONNECTED}.
     * @param rawCapabilities The raw unparsed audio capabilities.
     * @param portId The port ID for which the state change is being reported
     */
    void onCapabilitiesReported(in byte[] rawCapabilities, in int portId);
}
