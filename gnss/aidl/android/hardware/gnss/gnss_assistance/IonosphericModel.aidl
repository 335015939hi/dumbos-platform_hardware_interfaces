package android.hardware.gnss.gnss_assistance;

/**
 * Contains Galileo ionospheric model.
 * TODO: Defined in Galileo OS SIS ICD v2.1, 5.1.6?
 */
@VintfStability
parcelable GalileoIonosphericModel {
    double ai0;
    double ai1;
    double ai2;
    long transmissionTime;
}

/**
 * Contains Klobuchar ionospheric model used by GPS, BDS, QZSS.
 * TODO: Defined in ?
 */
@VintfStability
parcelable KlobucharIonosphericModel {
    /**
    * Klobuchar cefficients broadcast in satellite navigation message needed
    * to convert into correction parameters ? TODO: do we need this comment?
    */
    double alpha0;
    double alpha1;
    double alpha2;
    double alpha3;
    double beta0;
    double beta1;
    double beta2;
    double beta3;
    long transmissionTime;
}