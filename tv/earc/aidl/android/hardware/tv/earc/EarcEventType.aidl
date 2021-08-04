package android.hardware.tv.earc;

/**
 * HDMI EARC All event type.
 * Currently it's only the status/capability/latency change event.
 */
@VintfStability
@Backing(type="int")
enum EarcEventType {
    STATUS_CHG = 0,
    CAPABILITY_CHG,
    LATENCY_CHG,
}
