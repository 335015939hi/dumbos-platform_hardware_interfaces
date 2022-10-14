// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FieldSupportedValues;
import android.hardware.media.c2.Status;

/**
 * This structure is used to hold the result from
 * IConfigurable::querySupportedValues().
 */
@VintfStability
parcelable FieldSupportedValuesQueryResult {
    /**
     * Result of the query. Possible values are
     * - `OK`: The query was successful.
     * - `BAD_STATE`: The query was requested when the `IConfigurable` instance
     *   was in a bad state.
     * - `BAD_INDEX`: The requested field was not recognized.
     * - `TIMED_OUT`: The query could not be completed in a timely manner.
     * - `BLOCKING`: The query must block, but the parameter `mayBlock` in the
     *   call to `querySupportedValues()` was `false`.
     * - `CORRUPTED`: Some unknown error occurred.
     */
    Status status;
    /**
     * Supported values. This is meaningful only when #status is `OK`.
     */
    FieldSupportedValues values;
}
