package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.GpsSatelliteClockModel;
import android.hardware.gnss.gnss_assistance.OrbitModel;
import android.hardware.gnss.gnss_assistance.SatelliteEphemerisTime;
import android.hardware.gnss.gnss_assistance.DGnssCorrection;

/**
 * Contains information about L2 params
 * TODO: Defined in ?
 */
@VintfStability
parcelable GpsL2Params {
    int l2Code;
    int l2Flag;
}

/**
 * Contains information about GPS health. The information is tied to
 * Legacy Navigation (LNAV) data, not Civil Navigation (CNAV) data.
 */
@VintfStability
parcelable GpsSatelliteHealth {
    /**
     * Represents "SV health" in the "BROADCAST ORBIT - 6"
     * record of RINEX 3.05. Table A6, pp.68.
     */
    int svHealth;

    /**
     * Represents "SV accuracy" in meters in the "BROADCAST ORBIT - 6"
     * record of RINEX 3.05. Table A6, pp.68.
     */
    double svAccur;

    /**
     * Represents the "Fit Interval" in hours in the "BROADCAST ORBIT - 7"
     * record of RINEX 3.05. Table A6, pp.69.
     */
    double fitInt;
}

/**
 * Contains ephemeris parameters specific to GPS satellites.
 */
@VintfStability
parcelable GpsSatelliteEphemeris {
    /** Satellite PRN */
    int prn;

    /** L2 parameters */
    GpsL2Params gpsL2Params;

    /** Clock model */
    GpsSatelliteClockModel satelliteClockModel;

    /** Orbit model */
    KeplerianOrbitModel satelliteOrbitModel;

    /** Health */
    GpsSatelliteHealth gpsSatelliteHealth;

    /** Ephemeris time */
    SatelliteEphemerisTime satelliteEphemerisTime;
}

/**
 * GNSS corrections specific to GPS satellites.
 */
@VintfStability
parcelable GpsSatelliteCorrections {
    /** Satellite PRN */
    int prn;

    // Assuming DgnssSignalCorrection is defined elsewhere and converted to AIDL
    DgnssSignalCorrection[] signalCorrections;
}