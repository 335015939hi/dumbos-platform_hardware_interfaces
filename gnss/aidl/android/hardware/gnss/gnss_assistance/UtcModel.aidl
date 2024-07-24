package android.hardware.gnss.gnss_assistance;

/**
 * Contians parameters to convert from current GNSS time to UTC time.
 * TODO: Defined in ?
 */
@VintfStability
parcelable UtcModel {
    double a0;
    double a1;
    int timeOfWeek;
    int weekNumber;
    long transmissionTime;
}