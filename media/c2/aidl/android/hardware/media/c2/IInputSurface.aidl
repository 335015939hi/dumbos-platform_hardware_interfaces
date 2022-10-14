// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.IInputSink;
import android.hardware.media.c2.IInputSurfaceConnection;
import android.hardware.media.c2.Status;

/**
 * Input surface for a Codec2 component.
 *
 * An <em>input surface</em> is an instance of `IInputSurface`, which may be
 * created by calling IComponentStore::createInputSurface(). Once created, the
 * client may
 *   1. write data to it via the `IGraphicBufferProducer` interface; and
 *   2. use it as input to a Codec2 encoder.
 *
 * @sa IInputSurfaceConnection, IComponentStore::createInputSurface(),
 *     IComponent::connectToInputSurface().
 */
@VintfStability
interface IInputSurface {
    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Connects the input surface to an input sink.
     *
     * This function is generally called from inside the implementation of
     * IComponent::connectToInputSurface(), where @p sink is a thin wrapper of
     * the component that consumes buffers from this surface.
     *
     * @param sink Input sink. See `IInputSink` for more information.
     * @param out status Status of the call, which may be
     *   - `OK`        - Configuration successful.
     *   - `BAD_VALUE` - @p sink is invalid.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out connection `IInputSurfaceConnection` object. This must not be
     *     null if @p status is `OK`.
     */
    void connect(in IInputSink sink, out Status status, out IInputSurfaceConnection connection);

    // Adding return type to method instead of out param IConfigurable configurable since there is only one return value.
    /**
     * Returns the @ref IConfigurable instance associated to this input surface.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();

    // Adding return type to method instead of out param android.hardware.graphics.bufferqueue2.IGraphicBufferProducer producer since there is only one return value.
    /**
     * Returns the producer interface into the internal buffer queue.
     *
     * @return `IGraphicBufferProducer` instance. This must not be
     * null.
     */
    android.hardware.graphics.bufferqueue2.IGraphicBufferProducer getGraphicBufferProducer();
}
