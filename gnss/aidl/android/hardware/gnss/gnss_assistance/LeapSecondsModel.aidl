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

/**
 * Contains the leap seconds set of parameters needed for GNSS time.
 * TODO: Defined in ?
 */
@VintfStability
parcelable LeapSecondsModel {
    /** Time difference due to leap seconds before event. (UTC) */
    int leapSeconds;

    /** Time difference due to leap seconds after event. (UTC) */
    int leapSecondsFuture;

    /** Week number in which the leap second event will occur. (UTC) */
    int weekNumberLeapSecondsFuture;

    /** Day Number when the next leap second will occur. */
    int dayNumberLeapSecondsFuture;
}
