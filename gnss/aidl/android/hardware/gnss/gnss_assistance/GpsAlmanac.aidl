package android.hardware.gnss.gnss_assistance;

/**
 * Contains GPS Almanac data.
 * TODO: Defined in ?
 */
@VintfStability
parcelable GpsAlmanac {
    int gpsWeek;
    int secondsOfGpsWeek;
    GpsSatelliteAlmanac[] satelliteAlmanac;
}

@VintfStability
parcelable GpsSatelliteAlmanac {
    int prn;
    int svHealth;
    double eccentricity;
    double inclination;
    double omega;
    double omega0;
    double omegaDot;
    double rootA;
    double m0;
    double af0;
    double af1;
}