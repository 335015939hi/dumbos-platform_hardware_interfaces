package android.hardware.gnss.gnss_assistance;

/**
 * Contains the leap seconds set of parameters needed for GNSS time.
 * TODO: Defined in ?
 */
@VintfStability
parcelable LeapSecondsModel {
    /** Time difference due to leap seconds before event. (UTC) */
    int leapSeconds;

    /** Time difference due to leap seconds after event. (UTC) */
    int leapSecondsFuture;

    /** Week number in which the leap second event will occur. (UTC) */
    int weekNumberLeapSecondsFuture;

    /** Day Number when the next leap second will occur. */
    int dayNumberLeapSecondsFuture;
}