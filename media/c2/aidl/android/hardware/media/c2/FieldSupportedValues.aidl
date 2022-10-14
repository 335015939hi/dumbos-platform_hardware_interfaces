// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.ValueRange;

/*
 * Description of supported values for a field.
 *
 * This can be a continuous range or a discrete set of values.
 *
 * The intended type of values must be made clear in the context where
 * `FieldSupportedValues` is used.
 */
@VintfStability
union FieldSupportedValues {
    /**
     * No supported values
     */
    boolean empty;
    /**
     * Numeric range, described in a #ValueRange structure
     */
    ValueRange range;
    /**
     * List of values
     */
    long[] values;
    /**
     * List of flags that can be OR-ed
     */
    long[] flags;
}
