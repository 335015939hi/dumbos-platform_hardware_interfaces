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

import android.hardware.gnss.gnss_assistance.GnssInterval;
import android.hardware.gnss.gnss_assistance.GnssPseudorangeCorrection;

/**
 * Observation errors associated with a component (e.g. the Ionospheric error
 * component of SSR). In the case of OSR, there is only a single component
 * representing all errors observed by a station. In the future, this message
 * may be extended to support other observation types, e.g. phaserange
 * corrections.
 */
@VintfStability
parcelable DgnssCorrectionComponent {
    /**
     * Uniquely identifies the source of correction (e.g. "Klobuchar" for
     * ionospheric corrections or the station ID for OSR). Clients should not
     * depend on the value of the source key but, rather, can compare before/after
     * to detect changes.
     */
    String sourceKey;

    /**
    * Time interval referenced against the GPS epoch. The start must be less than
    * or equal to the end. When the start equals the end, the interval is empty.
    *
    * Using GPS time is a more natural format than UTC in the GNSS domain and
    * matches what is reported by GPS receivers via the Android GnssClock API:
    * https://developer.android.com/reference/android/location/GnssClock#getFullBiasNanos()
    *
    * GPS time also avoids leap second differences and smearing behavior inherent
    * in google.protobuf.Timestamp:
    * http://google3/google/protobuf/timestamp.proto?rcl=621612578&l=51-53
    * TODO: Review comment above
    * TODO: rename to GnssTimeInterval?
    */
    @VintfStability
    parcelable GnssInterval {
        /**
        * Inclusive start of the interval in milliseconds since the GPS epoch. A
        * timestamp matching this interval will have to be the same or after the
        * start. Required as a reference time for the initial correction value and
        * its rate of change over time.
        */
        long startMillisSinceGpsEpoch;

        /**
        * Exclusive end of the interval in milliseconds since the GPS epoch. If
        * specified, a timestamp matching this interval will have to be before the
        * end.
        */
        long endMillisSinceGpsEpoch;
    }

    /**
     * The correction is only applicable during this time interval.
     * TODO: Rename to validityInterval ?
     */
    GnssInterval validity;

    /**
     * Pseudorange correction.
     */
    @VintfStability
    parcelable DgnssPseudorangeCorrection {
        /**
        * Correction to be added to the measured pseudorange, in meters.
        */
        double correctionMeters;

        /**
        * Uncertainty of the correction, in meters.
        */
        double correctionUncertaintyMeters;

        /**
        * Linear approximation of the change in correction over time. Intended
        * usage is to adjust the correction using the formula:
        *   correctionMeters + correctionRateMetersPerSecond * delta_seconds
        * Where `delta_seconds` is the number of elapsed seconds since the beginning
        * of the correction validity interval.
        */
        double correctionRateMetersPerSecond;
    }

    /**
     * Pseudorange correction, in meters.
     */
    DgnssPseudorangeCorrection pseudorangeCorrection;
}
