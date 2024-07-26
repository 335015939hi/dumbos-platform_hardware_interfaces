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

import android.hardware.gnss.gnss_assistance.DgnssSignalCorrection;
import android.hardware.gnss.gnss_assistance.GalileoIonosphericModel
import android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.GpsAlmanac;
import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.GlonassAlmanac;
import android.hardware.gnss.gnss_assistance.GlonassSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.KlobucharIonosphericModel;
import android.hardware.gnss.gnss_assistance.QzssAlmanac;
import android.hardware.gnss.gnss_assistance.QzssSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.LeapSecondsModel;
import android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel;
import android.hardware.gnss.gnss_assistance.TimeModel;
import android.hardware.gnss.gnss_assistance.UtcModel;

/**
 * Contains GNSS assistance.
 *
 */
@VintfStability
parcelable GnssAssistance {

    /**
     * Contains GPS assistance.
     *
     */
    @VintfStability
    parcelable GpsAssistance {

        /** The GPS almanac. */
        GpsAlmanac almanac;

        /** The Klobuchar ionospheric model. */
        KlobucharIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of GPS ephemeris. */
        GpsSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /**
        * GNSS corrections specific to GPS satellites.
        */
        @VintfStability
        parcelable GpsSatelliteCorrections {
            /** Satellite PRN */
            int prn;

            /** GNSS Signal corrections */
            DgnssSignalCorrection[] signalCorrections;
        }

        /** The array of GPS satellite corrections. */
        GpsSatelliteCorrections[] satelliteCorrections;
    }

    /**
     * Contains Galileo assistance.
     *
     */
    @VintfStability
    parcelable GalileoAssistance {

        /** The Galileo almanac. */
        GalileoAlmanac almanac;

        /** The Galileo ionospheric model. */
        GalileoIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of Galileo ephemeris. */
        GalileoSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /**
        * GNSS corrections specific to Galileo satellites.
        */
        @VintfStability
        parcelable GalileoSatelliteCorrections {
            /** Satellite PRN */
            int prn;

            /** GNSS Signal corrections */
            DgnssSignalCorrection[] signalCorrections;
        }

        /** The array of Galileo satellite corrections. */
        GalileoSatelliteCorrections[] satelliteCorrections;
    }


    /**
     * Contains Glonass assistance.
     *
     */
    @VintfStability
    parcelable GlonassAssistance {

        /** The Glonass almanac. */
        GlonassAlmanac almanac;

        /** The UTC model. */
        UtcModel utcModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of Glonass ephemeris. */
        GlonassSatelliteEphemeris[] satelliteEphemeris;

        /**
         * GNSS corrections specific to Glonass satellites.
         */
        @VintfStability
        parcelable GlonassSatelliteCorrections {
            /**
            * L1/Satellite system (R), satellite number (slot number in sat.
            * constellation)
            */
            int slot;

            /** Glonass Signal corrections */
            DgnssSignalCorrection[] signalCorrections;
        }

        /** The array of Glonass satellite corrections. */
        GlonassSatelliteCorrections[] satelliteCorrections;
    }

    /**
     * Contains QZSS assistance.
     * TODO: QzssAssistance is the same as GpsAssistance, can we reuse GpsAssistance?
     */
    @VintfStability
    parcelable QzssAssistance {

        /** The QZSS almanac. */
        QzssAlmanac almanac;

        /** The Klobuchar ionospheric model. */
        KlobucharIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of GPS ephemeris. */
        QzssSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /**
         * QZSS corrections specific to QZSS satellites.
         */
        @VintfStability
        parcelable QzssSatelliteCorrections {
            /** Satellite PRN */
            int prn;

            /** QZSS Signal corrections */
            DgnssSignalCorrection[] signalCorrections;
        }

        /** The array of QZSS satellite corrections. */
        QzssSatelliteCorrections[] satelliteCorrections;
    }

    GpsAssistance gpsAssistance;
    GlonassAssistance gloAssistance;
    GalileoAssistance galAssistance;
    //BdsAssistance bdsAssistance;
    QzssAssistance qzsAssistance;
}
