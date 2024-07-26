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
 * Contains Keplerian orbit model parameters
 * TODO: Defined in ?
 */
@VintfStability
parcelable KeplerianOrbitModel {
    /** Square root of the semi-major axis (sqrt(m)) */
    double rootA;

    /** Eccentricity */
    double e;

    /** Inclination angle at reference time (radians) */
    double i0;

    /** Rate of inclination angle (radians/sec) */
    double iDot;

    /** Argument of perigee (radians) */
    double omega;

    /** Longitude of ascending node of orbit plane at beginning of week (radians) */
    double omega0;

    /** Rate of right ascension (radians/sec) */
    double omegaDot;

    /** Mean anomaly at reference time (radians) */
    double m0;

    /** Mean motion difference from computed value (radians/sec) */
    double deltaN;

    /**
    * Contains second-order harmonic perturbations
    */
    @VintfStability
    parcelable SecondOrderHarmonicPerturbation {
        /** Amplitude of Cosine Harmonic Correction Term to Angle of Inclination */
        double cic;

        /** Amplitude of Sine Harmonic Correction Term to the Angle of Inclination */
        double cis;

        /** Amplitude of Cosine Harmonic Correction Term to the Orbit Radius */
        double crc;

        /** Amplitude of Sine Harmonic Correction Term to the Orbit Radius */
        double crs;

        /** Amplitude of Cosine Harmonic Correction Term to the Argument of Latitude */
        double cuc;

        /** Amplitude of Sine Harmonic Correction Term to the Argument of Latitude */
        double cus;
    }

    /** Second-order harmonic perturbations */
    SecondOrderHarmonicPerturbation secondOrderHarmonicPerturbation;
}
