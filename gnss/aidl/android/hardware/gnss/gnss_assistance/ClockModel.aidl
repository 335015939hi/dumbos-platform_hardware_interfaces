package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.SatelliteClockType;
import android.hardware.gnss.gnss_assistance.TimeOfClock;

/*
 * Contains the set of parameters needed for Galileo satellite clock
 * correction.
 * This is defined in Galileo OS SIS ICD (mainly 46 - 48).
 */
 @VintfStability
parcelable GalileoSatelliteClockModel {
    /** Time of the clock */
    TimeOfClock toc;

    /*
    * SV clock bias correction coefficient (seconds)
    * TODO(justinowusu) Decide whether af0 should be double or int
    */
    double af0;

    /** SV clock drift correction coefficient (sec/sec) */
    double af1;

    /** SV clock drift rate correction coefficient (sec/sec^2) */
    double af2;

    /*
    * roadcast Group Delay (seconds)
    * TODO(justinowusu): Decide whether bgd should be double or int
    */
    double bgd;

    /** Signal in Space Accuracy */
    double sisa;

    /** Type of satellite clock */
    SatelliteClockType satelliteClockType;
}

/*
 * Contains the set of parameters needed for GPS satellite clock
 * correction.
 * TODO: This is defined in ?
 */
 @VintfStability
parcelable GpsSatelliteClockModel {
    /** Time of the clock */
    TimeOfClock timeOfClock;

    /** SV clock bias (seconds.) */
    double af0;

    /** SV clock drift (sec/sec) */
    double af1;

    /** Clock drift rate (sec/sec^2). */
    double af2;

    /** Group Delay Differential (seconds). */
    double tgd;

    /** Issue of Data (clock) */
    int iodc;
}

/*
 * Contains the set of parameters needed for Beidou satellite clock
 * correction.
 * TODO: This is defined in ?
 */
parcelable BeidouSatelliteClockModel {
    /** Time of the clock. */
    TimeOfClock timeOfClock;

    /** SV clock bias (seconds). */
    double af0;

    /** SV clock drift (sec/sec). */
    double af1;

    /** Clock drift rate (sec/sec^2). */
    double af2;

    /** Group Delay Differential 1 B1/B3 (seconds). */
    double tgd1;

    /** Group Delay Differential 2 B2/B3 (seconds). */
    double tgd2;

    /** Age of Data (clock). */
    int aodc;
}

/*
 * States the type of satellite clock
 */
@VintfStability
@Backing(type="int")
enum SatelliteClockType {
    UNDEFINED = 0,
    GALILEO_FNAV_CLOCK = 1,
    GALILEO_INAV_CLOCK = 2
}