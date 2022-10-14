// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FieldSupportedValues;
import android.hardware.media.c2.ParamField;

/**
 * Supported values for a field.
 *
 * This is a pair of the field specifier together with an optional supported
 * values object. This structure is used when reporting parameter configuration
 * failures and conflicts.
 */
@VintfStability
parcelable ParamFieldValues {
    /**
     * Reference to a field or a C2Param structure.
     */
    ParamField paramOrField;
    /**
     * Optional supported values for the field if #paramOrField specifies an
     * actual field that is numeric (non struct, blob or string). Supported
     * values for arrays (including string and blobs) describe the supported
     * values for each element (character for string, and bytes for blobs). It
     * is optional for read-only strings and blobs.
     */
    FieldSupportedValues[] values;
}
