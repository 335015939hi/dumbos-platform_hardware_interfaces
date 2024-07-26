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
import android.hardware.gnss.gnss_assistance.SatelliteEphemerisTime;

/**
 * Contains ephemeris parameters specific to Glonass satellites.
 */
@VintfStability
parcelable GlonassSatelliteEphemeris {

    /*
     * Contains the set of parameters needed for GPS satellite clock
     * correction.
     */
    @VintfStability
    parcelable GlonassSatelliteClockModel {
        /** L1/Epoch: Toc - Time of Clock (UTC) year (4 digits) month, day, hour, minute, second */
        TimeOfClock timeOfClock;

        /** L1/SV clock bias (sec) (-TauN) */
        double clockBias;

        /** L1/SV relative frequency bias (+GammaN) */
        double freqBias;

        /** L3/frequency number(-7...+13) (-7...+6 ICD 5.1) */
        int freqNumber;
    }

    /**
     * Contains Glonass orbit model parameters.
     */
    @VintfStability
    parcelable GlonassSatelliteOrbitModel {
        /** L2/Satellite position X(km) */
        double x;

        /** L2/velocity X dot(km/sec) */
        double xDot;

        /** L2/X acceleration (km/sec2) */
        double xAccel;

        /** L2/Satellite position Y(km) */
        double y;

        /** L2/velocity Y dot(km/sec) */
        double yDot;

        /** L2/Y acceleration  (km/sec2) */
        double yAccel;

        /** L2/Satellite position Z(km) */
        double z;

        /** L2/velocity Z dot(km/sec) */
        double zDot;

        /** L2/Z acceleration  (km/sec2) */
        double zAccel;
    }

    /** L1/Satellite system (R), satellite number (slot number in sat. constellation) */
    int slot;

    /** L2/health (0=healthy, 1=unhealthy)(Bn) */
    int health;

    /** L1/Message frame time (tk+nd*86400) in seconds of the UTC week */
    double frameTime;

    /** L4/Age of oper. information (days)(E) */
    double ageInDays;

    GlonassSatelliteClockModel satelliteClockModel;
    GlonassSatelliteOrbitModel satelliteOrbitModel;
}
