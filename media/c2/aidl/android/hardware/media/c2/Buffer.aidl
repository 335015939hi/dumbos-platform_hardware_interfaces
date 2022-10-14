// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.Block;

/**
 * A codec buffer, which is a collection of @ref Block objects and metadata.
 *
 * This is a part of @ref FrameData.
 */
@VintfStability
parcelable Buffer {
    /**
     * Metadata associated with the buffer.
     */
    byte[] info;
    /**
     * Blocks contained in the buffer.
     */
    Block[] blocks;
}
