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

import android.hardware.bluetooth.ranging.Nadm;
import android.hardware.bluetooth.ranging.RttToaTodData;

/**
 * Raw ranging data of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable ModeOneData {
    /**
     * bits 0 to 3:
     * ** 0x0 = CS Access Address check is successful, and all bits match the expected sequence
     * ** 0x1 = CS Access Address check contains one or more bit errors
     * ** 0x2 = CS Access Address not found
     * bits 4 to 7: Number of bit errors being reported on the payload with a random or sounding
     * sequence. Value 0 may indicate zero bit errors or no report available.
     * Value 15 may indicate 15 or more bit errors.
     */
    byte packetQuality;
    /**
     * Normalized Attack Detector Metric.
     */
    Nadm packetNadm;
    /**
     * Range: -127 to +20
     * Unit: dBm
     * Value: 0x7F - RSSI is not available
     */
    byte packetRssi;
    /**
     * Time difference of the time of arrival and the time of depature of the CS packets.
     * see RttToaTodData for details.
     */
    RttToaTodData rttToaTodData;
    /**
     * Antenna identifier used for the RTT packet
     * Value: 0x01 to 0x04
     */
    byte packetAntenna;
    /**
     * 0x00XXXXXX
     * ** Phase Correction Term (bits 0 to 11 are the I sample with type sint12,
     * ** bits 12 to 23 are the Q sample with type sint12
     * 0xFFFFFFFF - Phase Correction Term is not available
     */
    @nullable byte[4] packetPct1;
    /**
     * 0x00XXXXXX
     * ** Phase Correction Term (bits 0 to 11 are the I sample with type sint12,
     * ** bits 12 to 23 are the Q sample with type sint12
     * 0xFFFFFFFF - Phase Correction Term is not available
     */
    @nullable byte[4] packetPct2;
}
