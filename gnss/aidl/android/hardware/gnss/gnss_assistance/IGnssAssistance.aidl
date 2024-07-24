package android.hardware.gnss;

import android.hardware.gnss.gnss_assistance.GnssAssistance;
import android.hardware.gnss.gnss_assistance.IGnssAssistanceCallback;

/**
 * Interface for GNSS assistance data
 *
 */
@VintfStability
interface IGnssAssistance {

    /**
     * Inject the GNSS assistance into the GNSS receiver.
     *
     * @param gnssAssistance GNSS assistance.
     */
    void injectGnssAssistance(in GnssAssistance gnssAssistance);

    /**
     * Provides the callback routines to request the GNSS assistance.
     *
     * @param callback Handle to the IGnssAssistanceCallback interface.
     */
    void setCallback(in IGnssAssistanceCallback callback);
}