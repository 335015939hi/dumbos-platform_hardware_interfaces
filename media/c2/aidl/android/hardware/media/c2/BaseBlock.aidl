// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;


/**
 * Storage type for `BaseBlock`.
 *
 * A `BaseBlock` is a representation of a codec memory block. Coded data,
 * decoded data, codec-specific data, and other codec-related data are all sent
 * in the form of BaseBlocks.
 */
@VintfStability
union BaseBlock {
    /**
     * #nativeBlock is the opaque representation of a buffer.
     */
    android.os.NativeHandle nativeBlock;
    /**
     * #pooledBlock is a reference to a buffer handled by a BufferPool.
     */
    android.hardware.media.bufferpool2.BufferStatusMessage pooledBlock;
}
