package android.hardware.tv.earc;

/**
 * For user control HDMI EARC feature.
 */
@VintfStability
@Backing(type="int")
enum EarcControl {
    NO_WAY = 0,
    ARC_ONLY,
    PREFER_EARC,
}
