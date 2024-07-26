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
import android.hardware.gnss.gnss_assistance.KeplerianOrbitModel;

/**
 * Contains ephemeris parameters specific to Beidou satellites.
 */
@VintfStability
parcelable BeidouSatelliteEphemeris {

    /*
    * Contains the set of parameters needed for Beidou satellite clock
    * correction.
    * TODO: This is defined in ?
    */
    @VintfStability
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

    /**
     * Contains information about Beidou health.
     */
    parcelable BeidouSatelliteHealth {
        /**
         * Field sat_h1 stands for "SatH1" in the "BROADCAST ORBIT - 6" record of
         * RINEX 3.05 Table A14, pp.78.
         * It occupies bit 0. “0” means broadcasting satellite is good and “1”
         * means not.
         */
        int satH1; // Renamed for better readability

        /**
         * Field sv_accur stands for "SV accuracy" in meters in the
         * "BROADCAST ORBIT - 6" record of RINEX 3.05 Table A14, pp.78.
         */
        double svAccur; // Renamed for better readability
    }

    /**
    * Proto to store information about time of ephemeris
    * TODO: Reuse SatelliteEphemerisTime?
    */
    parcelable BeidouSatelliteEphemerisTime {
        /** Age of Data Ephemeris */
        int aode;

        /** Reference week number */
        int week;

        /** Time of Ephemeris */
        int toe;

        /** Transmission time of message */
        double transmissionTime;
    }

    /** Satellite PRN */
    int prn;

    /** Satellite clock model */
    BeidouSatelliteClockModel satelliteClockModel;

    /** Satellite orbit model */
    KeplerianOrbitModel satelliteOrbitModel;

    /** Satellite health */
    BeidouSatelliteHealth satelliteHealth;

    /** Satellite ephemeris time */
    BeidouSatelliteEphemerisTime satelliteEphemerisTime;
}
