// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.Buffer;
import android.hardware.media.c2.InfoBuffer;
import android.hardware.media.c2.WorkOrdinal;

/**
 * Data for an input frame or an output frame.
 *
 * This structure represents a @e frame with its metadata. A @e frame consists
 * of an ordered set of buffers, configuration changes, and info buffers along
 * with some non-configuration metadata.
 *
 * @note `FrameData` is the HIDL counterpart of `C2FrameData` in the Codec 2.0
 * standard.
 */
@VintfStability
parcelable FrameData {
    @VintfStability
    @Backing(type="int")
    enum Flags {
        /**
         * For input frames: no output frame shall be generated when processing
         * this frame, but metadata must still be processed.
         *
         * For output frames: this frame must be discarded but metadata is still
         * valid.
         */
        DROP_FRAME = (1 << 0),
        /**
         * This frame is the last frame of the current stream. Further frames
         * are part of a new stream.
         */
        END_OF_STREAM = (1 << 1),
        /**
         * This frame must be discarded with its metadata.
         *
         * This flag is only set by components, e.g. as a response to the flush
         * command.
         */
        DISCARD_FRAME = (1 << 2),
        /**
         * This frame is not the last frame produced for the input.
         *
         * This flag is normally set by the component - e.g. when an input frame
         * results in multiple output frames, this flag is set on all but the
         * last output frame.
         *
         * Also, when components are chained, this flag should be propagated
         * down the work chain. That is, if set on an earlier frame of a
         * work-chain, it should be propagated to all later frames in that
         * chain. Additionally, components down the chain could set this flag
         * even if not set earlier, e.g. if multiple output frames are generated
         * at that component for the input frame.
         */
        FLAG_INCOMPLETE = (1 << 3),
        /**
         * This frame contains only codec-specific configuration data, and no
         * actual access unit.
         *
         * @deprecated Pass codec configuration with the codec-specific
         * configuration info together with the access unit.
         */
        CODEC_CONFIG = (1u << 31),
    }
    /**
     * Frame flags, as described in #Flags.
     */
    Flags flags;
    /**
     * @ref WorkOrdinal of the frame.
     */
    WorkOrdinal ordinal;
    /**
     * List of frame buffers.
     */
    Buffer[] buffers;
    /**
     * List of configuration updates.
     */
    byte[] configUpdate;
    /**
     * List of info buffers.
     */
    InfoBuffer[] infoBuffers;
}
