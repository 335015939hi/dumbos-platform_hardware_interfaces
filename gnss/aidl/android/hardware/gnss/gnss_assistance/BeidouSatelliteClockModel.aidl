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
 * Contains the set of parameters needed for Beidou satellite clock
 * correction.
 * TODO: This is defined in ?
 */
parcelable BeidouSatelliteClockModel {
    /** Time of the clock. */
    TimeOfClock timeOfClock;

    /** SV clock bias in seconds. */
    double af0;

    /** SV clock drift in seconds per second. */
    double af1;

    /** Clock drift rate in seconds per second squared. */
    double af2;

    /** Group delay differential 1 B1/B3 in seconds. */
    double tgd1;

    /** Group delay differential 2 B2/B3 in seconds. */
    double tgd2;

    /** Age of data (clock). */
    int aodc;
}
