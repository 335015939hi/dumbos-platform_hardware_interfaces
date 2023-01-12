// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.gatekeeper;

/**
 * Gatekeeper response codes; success >= 0; error < 0
 */
@VintfStability
@Backing(type="int")
enum GatekeeperStatusCode {
    STATUS_REENROLL = 1,
    STATUS_OK = 0,
    ERROR_GENERAL_FAILURE = -1,
    ERROR_RETRY_TIMEOUT = -2,
    ERROR_NOT_IMPLEMENTED = -3,
}
