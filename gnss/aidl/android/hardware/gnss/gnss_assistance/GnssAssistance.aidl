package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.IonosphericModel.KlobucharIonosphericModel;
import android.hardware.gnss.gnss_assistance.TimeModel;
import android.hardware.gnss.gnss_assistance.UtcModel;

/**
 * Contains GNSS assistance.
 *
 */
@VintfStability
parcelable GnssAssistance {
    GpsAssistance gpsAssistance;
    //GlonassAssistance gloAssistance;
    //GalileoAssistance galAssistance;
    //BdsAssistance bdsAssistance;
    //QzssAssistance qzsAssistance;
}

/**
 * Contains GPS assistance.
 *
 */
@VintfStability
parcelable GpsAssistance {

    /** The GPS almanac. */
    GpsAlmanac almanac;

    /** The Klobuchar ionospheric model. */
    KlobucharIonosphericModel ionospheric_model;

    /** The UTC model. */
    UtcModel utc_model;

    /** The leap seconds model. */
    LeapSecondsModel leap_seconds_model;

    /** The array of time models. */
    TimeModel[] time_models;

    /** The array of GPS ephemeris. */
    GpsSatelliteEphemeris[] satellite_ephemeris;

    /** The array of real time integrity models. */
    RealTimeIntegrityModel[] real_time_integrity_models;

    /** The array of GPS satellite corrections. */
    GpsSatelliteCorrections[] satellite_corrections;
}