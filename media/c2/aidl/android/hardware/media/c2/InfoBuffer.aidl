// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.Buffer;

/**
 * An extension of @ref Buffer that also contains a C2Param structure index.
 *
 * This is a part of @ref FrameData.
 */
@VintfStability
parcelable InfoBuffer {
    /**
     * A C2Param structure index.
     */
    int index;
    /**
     * Associated @ref Buffer object.
     */
    Buffer buffer;
}
