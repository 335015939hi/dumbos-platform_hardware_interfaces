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

package android.hardware.radio.config;

/**
 * Contains the device capabilities with respect to the Radio HAL.
 */
@VintfStability
parcelable HalDeviceCapabilities {
    /**
     * True indicates that the modem does NOT support the following features:
     * <ul>
     *   <li>
     *     Providing either LinkCapacityEstimate#secondaryDownlinkCapacityKbps or
     *     LinkCapacityEstimate:secondaryUplinkCapacityKbps when given from
     *     RadioIndication:currentLinkCapacityEstimate
     *   </li><li>
     *     Calling IRadio.setNrDualConnectivityState or querying IRadio.isNrDualConnectivityEnabled
     *   </li><li>
     *     Requesting IRadio.setDataThrottling()
     *   </li>
     * </ul>
     */
    boolean modemReducedFeatureSet1;
}
