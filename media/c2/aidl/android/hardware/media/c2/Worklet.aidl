// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FrameData;
import android.hardware.media.c2.SettingResult;

/**
 * In/out structure containing some instructions for and results from output
 * processing.
 *
 * This is a part of @ref Work. One `Worklet` corresponds to one output
 * @ref FrameData. The client must construct an original `Worklet` object inside
 * a @ref Work object for each expected output before calling
 * IComponent::queue().
 */
@VintfStability
parcelable Worklet {
    /**
     * Component id. (Input)
     *
     * This is used only when tunneling is enabled.
     *
     * When used, this must match the return value from IConfigurable::getId().
     */
    int componentId;
    /**
     * List of C2Param objects describing tunings to be applied before
     * processing this `Worklet`. (Input)
     */
    byte[] tunings;
    /**
     * List of failures. (Output)
     */
    SettingResult[] failures;
    /**
     * Output frame data. (Output)
     */
    FrameData output;
}
