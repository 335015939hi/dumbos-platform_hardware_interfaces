// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Ordering information of @ref FrameData objects. Each member is used for
 * comparing urgency: a smaller difference from a reference value indicates that
 * the associated Work object is more urgent. The reference value for each
 * member is initialized the first time it is communicated between the client
 * and the codec, and it may be updated to later values that are communicated.
 *
 * Each member of `WorkOrdinal` is stored as an unsigned integer, but the actual
 * order it represents is derived by subtracting the reference value, then
 * interpreting the result as a signed number with the same storage size (using
 * two's complement).
 *
 * @note `WorkOrdinal` is the HIDL counterpart of `C2WorkOrdinalStruct` in the
 * Codec 2.0 standard.
 */
@VintfStability
parcelable WorkOrdinal {
    /**
     * Timestamp in microseconds.
     */
    long timestampUs;
    /**
     * Frame index.
     */
    long frameIndex;
    /**
     * Component specific frame ordinal.
     */
    long customOrdinal;
}
