// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FieldId;

/**
 * Reference to a field in a C2Param structure.
 */
@VintfStability
parcelable ParamField {
    /**
     * Index of the C2Param structure.
     */
    int index;
    /**
     * Identifier of the field inside the C2Param structure.
     */
    FieldId fieldId;
}
