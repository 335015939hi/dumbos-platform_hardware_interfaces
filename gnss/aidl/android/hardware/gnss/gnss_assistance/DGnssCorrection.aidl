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
 * Contains DGNSS correction.
 *
 */
@VintfStability
parcelable DgnssSignalCorrection {
    /**
    * Differentiates signals from the same satellite, e.g. GPS L1/L5
    */
    long carrierFrequencyHz;

    /**
     * The GNSS signal's code type. For OSR corrections, this is the code type
     * observed by the reference station. For SSR corrections, the code type is
     * optional. If present, it reflects what was used during computation of the
     * correction though, in practice, does not affect the output and may be
     * ignored.
     * See:
     * https://developer.android.com/reference/android/location/GnssMeasurement.html#getCodeType()
     * TODO: Change type to GnssSignalType ?
     */
    String codeType;

    /**
     * State Space Representation providing estimates of the different error
     * components.
     */
    SsrCorrection ssr;

    /**
     * Observation Space Representation providing the sum of all errors observed
     * by a reference station. Each element reports the OSR corrections from a
     * single reference station. Empty indicates that there weren't any available
     * reference stations near the receiver. When multiple reference stations
     * are nearby, the client may select the preferred station.
     */
    OsrCorrection[] osr;
}

/**
 * State Space Representation providing estimates of the different error
 * components. In the future, this message may be extended to support
 * clock/orbit components.
 */
@VintfStability
parcelable SsrCorrection {
    /**
     * Ionospheric component of total error.
     */
    DgnssCorrectionComponent ionosphere;

    /**
     * Tropospheric component of total error.
     */
    DgnssCorrectionComponent troposphere;
}

/**
 * Observation Space Representation providing the sum of all errors observed
 * by a reference station.
 */
@VintfStability
parcelable OsrCorrection {
    /**
     * Sum of all errors observed by a reference station.
     */
    DgnssCorrectionComponent pathDelay;
}

/**
 * Observation errors associated with a component (e.g. the Ionospheric error
 * component of SSR). In the case of OSR, there is only a single component
 * representing all errors observed by a station. In the future, this message
 * may be extended to support other observation types, e.g. phaserange
 * corrections.
 * TODO: Rename to DgnssCorrection ?
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
     * The correction is only applicable during this time interval.
     */
    GnssInterval validity;

    /**
     * Pseudorange correction, in meters.
     */
    DgnssPseudorangeCorrection pseudorange;
}

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
 */TODO: Review comment above
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
