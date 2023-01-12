// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.gatekeeper;

import android.hardware.gatekeeper.GatekeeperStatusCode;

/**
 * Gatekeeper response to any/all requests has this structure as mandatory part
 */
@VintfStability
parcelable GatekeeperResponse {
    /**
     * request completion status
     */
    GatekeeperStatusCode code;
    /**
     * retry timeout in ms, if code == ERROR_RETRY_TIMEOUT
     * otherwise unused (0)
     */
    int timeout;
    /**
     * optional crypto blob. Opaque to Android system.
     */
    byte[] data;
}
