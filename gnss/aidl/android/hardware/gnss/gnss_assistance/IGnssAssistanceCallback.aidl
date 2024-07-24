package android.hardware.gnss.gnss_assistance;


/**
 * Interface for GNSS assistance data request
 *
 */
@VintfStability
interface IGnssAssistanceCallback {

    /**
     * Callback to request the client to download GNSS assistance data
     *
     * @param callback Handle to the IGnssSuplCallback interface.
     */
    void downloadRequestCb();
}
