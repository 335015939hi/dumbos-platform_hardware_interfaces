// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.BaseBlock;
import android.hardware.media.c2.Work;

/**
 * List of `Work` objects.
 *
 * `WorkBundle` is used in IComponent::queue(), IComponent::flush() and
 * IComponentListener::onWorkDone(). A `WorkBundle` object consists of a list of
 * `Work` objects and a list of `BaseBlock` objects. Bundling multiple `Work`
 * objects together provides two benefits:
 *   1. Batching of `Work` objects can reduce the number of IPC calls.
 *   2. If multiple `Work` objects contain `Block`s that refer to the same
 *      `BaseBlock`, the number of `BaseBlock`s that is sent between processes
 *      is also reduced.
 *
 * @note `WorkBundle` is the HIDL counterpart of the vector of `C2Work` in the
 * Codec 2.0 standard. The presence of #baseBlocks helps with minimizing the
 * data transferred over an IPC.
 */
@VintfStability
parcelable WorkBundle {
    /**
     * A list of Work items.
     */
    Work[] works;
    /**
     * A list of blocks indexed by elements of #works.
     */
    BaseBlock[] baseBlocks;
}
