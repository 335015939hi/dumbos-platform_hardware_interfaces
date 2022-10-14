// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FieldDescriptor;

/**
 * Description of a C2Param structure. It consists of an index and a list of
 * `FieldDescriptor`s.
 */
@VintfStability
parcelable StructDescriptor {
    /**
     * Index of the structure.
     */
    int type;
    /**
     * List of fields in the structure.
     *
     * Fields are ordered by their offsets. A field that is a structure is
     * ordered before its members.
     */
    FieldDescriptor[] fields;
}
