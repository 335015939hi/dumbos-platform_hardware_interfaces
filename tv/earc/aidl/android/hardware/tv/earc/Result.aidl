package android.hardware.tv.earc;

/**
 * HDMI EARC result code.
 */
@VintfStability
@Backing(type="int")
enum Result {
    OK = 0,
    EARC_NOT_SUPPORT,
    INVALID_ARG,
    NO_RESPONED,
    UNKNOWN,
}
