// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.Status;
import android.hardware.media.c2.WorkBundle;

/**
 * An `IInputSink` is a receiver of work items.
 *
 * An @ref IComponent instance can present itself as an `IInputSink` via a thin
 * wrapper.
 *
 * @sa IInputSurface, IComponent.
 */
@VintfStability
interface IInputSink {
    // Adding return type to method instead of out param IConfigurable configurable since there is only one return value.
    /**
     * Returns the @ref IConfigurable instance associated to this sink.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Feeds work to the sink.
     *
     * @param workBundle `WorkBundle` object containing a list of `Work` objects
     *     to queue to the component.
     * @return Status of the call, which may be
     *   - `OK`        - Works in @p workBundle were successfully queued.
     *   - `BAD_INDEX` - Some component id in some `Worklet` is not valid.
     *   - `CANNOT_DO` - Tunneling has not been set up for this sink, but some
     *                   `Work` object contains tunneling information.
     *   - `NO_MEMORY` - Not enough memory to queue @p workBundle.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status queue(in WorkBundle workBundle);
}
