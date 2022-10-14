// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.Status;

/**
 * Connection between a component and an input surface.
 *
 * An instance of `IInputSurfaceConnection` contains an `IConfigurable`
 * interface for querying and configuring properties of the connection.
 */
@VintfStability
interface IInputSurfaceConnection {
    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Destroys the connection between an input surface and a component.
     *
     * @return Status of the call, which may be
     *   - `OK`        - The disconnection succeeded.
     *   - `BAD_STATE` - The component is not in running state.
     *   - `NOT_FOUND` - The surface is not connected to a component.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status disconnect();

    // Adding return type to method instead of out param IConfigurable configurable since there is only one return value.
    /**
     * Returns the @ref IConfigurable instance associated to this connection.
     *
     * This can be used to customize the connection.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();
}
