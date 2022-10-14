// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Reference to a @ref BaseBlock within a @ref WorkBundle.
 *
 * `Block` contains additional attributes that `BaseBlock` does not. These
 * attributes may differ among `Block` objects that refer to the same
 * `BaseBlock` in the same `WorkBundle`.
 */
@VintfStability
parcelable Block {
    /**
     * Identity of a `BaseBlock` within a `WorkBundle`. This is an index into
     * #WorkBundle.baseBlocks.
     */
    int index;
    /**
     * Metadata associated with this `Block`.
     */
    byte[] meta;
    /**
     * Fence for synchronizing `Block` access.
     */
    android.os.NativeHandle fence;
}
