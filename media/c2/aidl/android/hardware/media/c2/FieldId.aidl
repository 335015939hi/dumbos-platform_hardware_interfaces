// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Identifying information of a field relative to a known C2Param structure.
 *
 * Within a given C2Param structure, each field is uniquely identified by @ref
 * FieldId.
 */
@VintfStability
parcelable FieldId {
    /**
     * Offset of the field in bytes.
     */
    int offset;
    /**
     * Size of the field in bytes.
     */
    int size;
}
