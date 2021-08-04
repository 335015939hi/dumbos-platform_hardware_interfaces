package android.hardware.tv.earc;

/**
 * HDMI EARC connect status.
 * As specified with the Discovery and Disconnect State Diagram
 * in Figure 9-22/3 of the HDMI2.1 spec.
 */
@VintfStability
@Backing(type="int")
enum EarcStatus {
    EARC_IDLE = 0,
    EARC_WAITING,
    EARC_NOT_ENABLED,
    EARC_ENABLED,
}
