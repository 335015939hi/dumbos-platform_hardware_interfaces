package android.hardware.gnss.gnss_assistance;

/**
 * Contains time of ephemeris.
 * TODO: Defined in ?
 */
@VintfStability
parcelable SatelliteEphemerisTime {
    int iode = 1;
    int week = 2;
    int toe = 3;
    long transmissionTime = 4; //TODO: long or double?
}