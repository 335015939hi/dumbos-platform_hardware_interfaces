// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.IComponentInterface;
import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.IInputSink;
import android.hardware.media.c2.IInputSurface;
import android.hardware.media.c2.IInputSurfaceConnection;
import android.hardware.media.c2.Status;
import android.hardware.media.c2.WorkBundle;
import android.hardware.media.c2.SurfaceSyncObj;

/**
 * Interface for a Codec2 component corresponding to API level 1.2 or below.
 * Components have two states: stopped and running. The running state has three
 * sub-states: executing, tripped and error.
 *
 * All methods in `IComponent` must not block. If a method call cannot be
 * completed in a timely manner, it must return `TIMED_OUT` in the return
 * status.
 *
 * @note This is an extension of version 1.1 of `IComponent`. The purpose of the
 * extension is to add blocking allocation of output buffer from surface.
 */
// Interface inherits from android.hardware.media.c2@1.1::IComponent but AIDL does not support interface inheritance (methods have been flattened).
@VintfStability
interface IComponent {
    // Adding return type to method instead of out param IInputSink sink since there is only one return value.
    /**
     * Returns an @ref IInputSink instance that has the component as the
     * underlying implementation.
     *
     * @return `IInputSink` instance.
     */
    IInputSink asInputSink();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Configures a component for a tunneled playback mode.
     *
     * A successful call to this method puts the component in the *tunneled*
     * mode. In this mode, the output `Worklet`s returned in
     * IComponentListener::onWorkDone() may not contain any buffers. The output
     * buffers are passed directly to the consumer end of a buffer queue whose
     * producer side is configured with the returned @p sidebandStream passed
     * to IGraphicBufferProducer::setSidebandStream().
     *
     * The component is initially in the non-tunneled mode by default. The
     * tunneled mode can be toggled on only before the component starts
     * processing. Once the component is put into the tunneled mode, it shall
     * stay in the tunneled mode until and only until reset() is called.
     *
     * @param avSyncHwId A resource ID for hardware sync. The generator of sync
     *     IDs must ensure that this number is unique among all services at any
     *     given time. For example, if both the audio HAL and the tuner HAL
     *     support this feature, sync IDs from the audio HAL must not clash
     *     with sync IDs from the tuner HAL.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully. In this case,
     *                   @p sidebandHandle shall not be a null handle.
     *   - `OMITTED`   - The component does not support video tunneling.
     *   - `BAD_STATE` - The component is already running.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out sidebandHandle Codec-allocated sideband stream handle. This can
     *     be passed to IGraphicBufferProducer::setSidebandStream() to
     *     establish a direct channel to the consumer.
     */
    void configureVideoTunnel(in int avSyncHwId,
        out Status status, out android.os.NativeHandle sidebandHandle);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Starts using an input surface.
     *
     * The component must be in running state.
     *
     * @param inputSurface Input surface to connect to.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `CANNOT_DO` - The component does not support an input surface.
     *   - `BAD_STATE` - The component is not in running state.
     *   - `DUPLICATE` - The component is already connected to an input surface.
     *   - `REFUSED`   - The input surface is already in use.
     *   - `NO_MEMORY` - Not enough memory to start the component.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out connection `IInputSurfaceConnection` object, which can be used to
     *     query and configure properties of the connection. This cannot be
     *     null.
     */
    void connectToInputSurface(in IInputSurface inputSurface,
        out Status status, out IInputSurfaceConnection connection);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Starts using an OMX input surface.
     *
     * The component must be in running state.
     *
     * This method is similar to connectToInputSurface(), but it takes an OMX
     * input surface (as a pair of `IGraphicBufferProducer` and
     * `IGraphicBufferSource`) instead of Codec2's own `IInputSurface`.
     *
     * @param producer Producer component of an OMX input surface.
     * @param source Source component of an OMX input surface.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `CANNOT_DO` - The component does not support an OMX input surface.
     *   - `BAD_STATE` - The component is not in running state.
     *   - `DUPLICATE` - The component is already connected to an input surface.
     *   - `REFUSED`   - The input surface is already in use.
     *   - `NO_MEMORY` - Not enough memory to start the component.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out connection `IInputSurfaceConnection` object, which can be used to
     *     query and configure properties of the connection. This cannot be
     *     null.
     */
    void connectToOmxInputSurface(
        in android.hardware.graphics.bufferqueue.IGraphicBufferProducer producer,
        in android.hardware.media.omx.IGraphicBufferSource source,
        out Status status, out IInputSurfaceConnection connection);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // FIXME: AIDL does not allow long to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Creates a local `C2BlockPool` backed by the given allocator and returns
     * its id.
     *
     * The returned @p blockPoolId is the only way the client can refer to a
     * `C2BlockPool` object in the component. The id can be passed to
     * setOutputSurface() or used in some C2Param objects later.
     *
     * The created `C2BlockPool` object can be destroyed by calling
     * destroyBlockPool(), reset() or release(). reset() and release() must
     * destroy all `C2BlockPool` objects that have been created.
     *
     * @param allocatorId Id of a `C2Allocator`.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `NO_MEMORY` - Not enough memory to create the pool.
     *   - `BAD_VALUE` - @p allocatorId is not recognized.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out blockPoolId Id of the created C2BlockPool object. This may be
     *     used in setOutputSurface() if the allocator
     * @param out configurable Configuration interface for the created pool. This
     *     must not be null.
     */
    void createBlockPool(in int allocatorId,
        out Status status, out long blockPoolId, out IConfigurable configurable);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Destroys a local block pool previously created by createBlockPool().
     *
     * @param blockPoolId Id of a `C2BlockPool` that was previously returned by
     *      createBlockPool().
     * @return Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `NOT_FOUND` - The supplied blockPoolId is not valid.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status destroyBlockPool(in long blockPoolId);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status Status since there is only one return value.
    /**
     * Stops using an input surface.
     *
     * The component must be in running state.
     *
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `CANNOT_DO` - The component does not support an input surface.
     *   - `BAD_STATE` - The component is not in running state.
     *   - `NOT_FOUND` - The component is not connected to an input surface.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status disconnectFromInputSurface();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Drains the component, and optionally downstream components. This is a
     * signalling method; as such it does not wait for any work completion.
     *
     * The last `Work` item is marked as "drain-till-here", so the component is
     * notified not to wait for further `Work` before it processes what is
     * already queued. This method can also be used to set the end-of-stream
     * flag after `Work` has been queued. Client can continue to queue further
     * `Work` immediately after this method returns.
     *
     * This method must be supported in running (including tripped) states.
     *
     * `Work` that is completed must be returned via
     * IComponentListener::onWorkDone().
     *
     * @param withEos Whether to drain the component with marking end-of-stream.
     * @return Status of the call, which may be
     *   - `OK`        - The drain request has been successfully recorded.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status drain(in boolean withEos);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Discards and abandons any pending `Work` items for the component.
     *
     * This method must be supported in running (including tripped) states.
     *
     * `Work` that could be immediately abandoned/discarded must be returned in
     * @p flushedWorkBundle. The order in which queued `Work` items are
     * discarded can be arbitrary.
     *
     * `Work` that could not be abandoned or discarded immediately must be
     * marked to be discarded at the earliest opportunity, and must be returned
     * via IComponentListener::onWorkDone(). This must be completed within
     * 500ms.
     *
     * @param out status Status of the call, which may be
     *   - `OK`        - The component has been successfully flushed.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out flushedWorkBundle `WorkBundle` object containing flushed `Work`
     *     items.
     */
    void flush(out Status status, out WorkBundle flushedWorkBundle);

    // Adding return type to method instead of out param IComponentInterface intf since there is only one return value.
    /**
     * Returns the @ref IComponentInterface instance associated to this
     * component.
     *
     * An @ref IConfigurable instance for the component can be obtained by calling
     * IComponentInterface::getConfigurable() on the returned @p intf.
     *
     * @return `IComponentInterface` instance. This must not be null.
     */
    IComponentInterface getInterface();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Queues up work for the component.
     *
     * This method must be supported in running (including tripped) states.
     *
     * It is acceptable for this method to return `OK` and return an error value
     * using the IComponentListener::onWorkDone() callback.
     *
     * @param workBundle `WorkBundle` object containing a list of `Work` objects
     *     to queue to the component.
     * @return Status of the call, which may be
     *   - `OK`        - Works in @p workBundle were successfully queued.
     *   - `BAD_INDEX` - Some component id in some `Worklet` is not valid.
     *   - `CANNOT_DO` - The components are not tunneled but some `Work` object
     *                   contains tunneling information.
     *   - `NO_MEMORY` - Not enough memory to queue @p workBundle.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status queue(in WorkBundle workBundle);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Releases the component.
     *
     * This method must be supported in stopped state.
     *
     * This method destroys the component. Upon return, if @p status is `OK` or
     * `DUPLICATE`, all resources must have been released.
     *
     * @return Status of the call, which may be
     *   - `OK`        - The component has been released.
     *   - `BAD_STATE` - The component is running.
     *   - `DUPLICATE` - The component is already released.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status release();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Resets the component.
     *
     * This method must be supported in all (including tripped) states other
     * than released.
     *
     * This method must be supported during any other blocking call.
     *
     * This method must return withing 500ms.
     *
     * When this call returns, if @p status is `OK`, all `Work` items must
     * have been abandoned, and all resources (including `C2BlockPool` objects
     * previously created by createBlockPool()) must have been released.
     *
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. For all other return values, the
     * component must be in the stopped state.
     *
     * This brings settings back to their default, "guaranteeing" no tripped
     * state.
     *
     * @return Status of the call, which may be
     *   - `OK`        - The component has been reset.
     *   - `BAD_STATE` - Component is in released state.
     *   - `DUPLICATE` - When called during another reset call from another
     *                   thread.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status reset();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Starts using a surface for output.
     *
     * This method must not block.
     *
     * @param blockPoolId Id of the `C2BlockPool` to be associated with the
     *     output surface.
     * @param surface Output surface.
     * @return Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `CANNOT_DO` - The component does not support an output surface.
     *   - `REFUSED`   - The output surface cannot be accessed.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status setOutputSurface(in long blockPoolId,
        in android.hardware.graphics.bufferqueue2.IGraphicBufferProducer surface);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Starts using a surface for output with a synchronization object
     *
     * This method must not block.
     *
     * @param blockPoolId Id of the `C2BlockPool` to be associated with the
     *     output surface.
     * @param surface Output surface.
     * @param syncObject synchronization object for buffer allocation between
     *     Framework and Component.
     * @return Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `CANNOT_DO` - The component does not support an output surface.
     *   - `REFUSED`   - The output surface cannot be accessed.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status setOutputSurfaceWithSyncObj(in long blockPoolId,
        in android.hardware.graphics.bufferqueue2.IGraphicBufferProducer surface,
        in SurfaceSyncObj syncObject);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Starts the component.
     *
     * This method must be supported in stopped state as well as tripped state.
     *
     * If the return value is `OK`, the component must be in the running state.
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. Otherwise, the component must be in
     * the stopped state.
     *
     * If a component is in the tripped state and start() is called while the
     * component configuration still results in a trip, start() must succeed and
     * a new onTripped() callback must be used to communicate the configuration
     * conflict that results in the new trip.
     *
     * @return Status of the call, which may be
     *   - `OK`        - The component has started successfully.
     *   - `BAD_STATE` - Component is not in stopped or tripped state.
     *   - `DUPLICATE` - When called during another start call from another
     *                   thread.
     *   - `NO_MEMORY` - Not enough memory to start the component.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status start();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Stops the component.
     *
     * This method must be supported in running (including tripped) state.
     *
     * This method must return withing 500ms.
     *
     * Upon this call, all pending `Work` must be abandoned.
     *
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. For all other return values, the
     * component must be in the stopped state.
     *
     * This does not alter any settings and tunings that may have resulted in a
     * tripped state.
     *
     * @return Status of the call, which may be
     *   - `OK`        - The component has stopped successfully.
     *   - `BAD_STATE` - Component is not in running state.
     *   - `DUPLICATE` - When called during another stop call from another
     *                   thread.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status stop();
}
