// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Common return values for Codec2 operations.
 */
@VintfStability
@Backing(type="int")
enum Status {
    /**
     * Operation completed successfully.
     */
    OK = 0,
    /**
     * Argument has invalid value (user error).
     */
    BAD_VALUE = -22,
    /**
     * Argument uses invalid index (user error).
     */
    BAD_INDEX = -75,
    /**
     * Argument/Index is valid but not possible.
     */
    CANNOT_DO = -2147483646,
    /**
     * Object already exists.
     */
    DUPLICATE = -17,
    /**
     * Object not found.
     */
    NOT_FOUND = -2,
    /**
     * Operation is not permitted in the current state.
     */
    BAD_STATE = -38,
    /**
     * Operation would block but blocking is not permitted.
     */
    BLOCKING = -9930,
    /**
     * Not enough memory to complete operation.
     */
    NO_MEMORY = -12,
    /**
     * Missing permission to complete operation.
     */
    REFUSED = -1,
    /**
     * Operation did not complete within timeout.
     */
    TIMED_OUT = -110,
    /**
     * Operation is not implemented/supported (optional only).
     */
    OMITTED = -74,
    /**
     * Some unexpected error prevented the operation.
     */
    CORRUPTED = -2147483648,
    /**
     * Status has not been initialized.
     */
    NO_INIT = -19,
}
