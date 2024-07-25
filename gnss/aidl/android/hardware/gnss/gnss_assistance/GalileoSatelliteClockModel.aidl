/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.TimeOfClock;

/*
 * States the type of satellite clock
 */
@VintfStability
@Backing(type="int")
enum SatelliteClockType {
    UNDEFINED = 0,
    GALILEO_FNAV_CLOCK = 1,
    GALILEO_INAV_CLOCK = 2
}

/*
 * Contains the set of parameters needed for Galileo satellite clock
 * correction.
 * This is defined in Galileo OS SIS ICD (mainly 46 - 48).
 */
 @VintfStability
parcelable GalileoSatelliteClockModel {
    /** Time of the clock */
    TimeOfClock toc;

    /*
     * SV clock bias correction coefficient in seconds.
     * TODO(justinowusu) Decide whether af0 should be double or int
     */
    double af0;

    /** SV clock drift correction coefficient in seconds per second. */
    double af1;

    /** SV clock drift rate correction coefficient in seconds per second
     * squared.
     */
    double af2;

    /*
     * Broadcast group delay in seconds.
     * TODO(justinowusu): Decide whether bgd should be double or int
     */
    double bgd;

    /** Signal in space accuracy */
    double sisa;

    /** Type of satellite clock */
    SatelliteClockType satelliteClockType;
}
