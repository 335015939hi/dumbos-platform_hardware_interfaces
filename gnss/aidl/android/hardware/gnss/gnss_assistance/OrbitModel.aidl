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

    /** Second-order harmonic perturbations */
    SecondOrderHarmonicPerturbation secondOrderHarmonicPerturbation;
}

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
