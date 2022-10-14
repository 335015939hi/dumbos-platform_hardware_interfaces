// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.Buffer;
import android.hardware.media.c2.IComponent;
import android.hardware.media.c2.IComponentInterface;
import android.hardware.media.c2.IComponentListener;
import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.IInputSurface;
import android.hardware.media.c2.Status;
import android.hardware.media.c2.StructDescriptor;
import android.hardware.media.c2.IComponent;
import android.hardware.media.c2.IComponent;

/**
 * Entry point for Codec2 HAL.
 *
 * All methods in `IComponentStore` must not block. If a method call cannot be
 * completed in a timely manner, it must return `TIMED_OUT` in the return
 * status. The only exceptions are getPoolClientManager() and getConfigurable(),
 * which must always return immediately.
 *
 * @note This is an extension of version 1.1 of `IComponentStore`. The purpose
 * of the extension is to add support for blocking output buffer allocator.
 */
// Interface inherits from android.hardware.media.c2@1.1::IComponentStore but AIDL does not support interface inheritance (methods have been flattened).
@VintfStability
interface IComponentStore {
    /**
     * Component traits.
     */
    @VintfStability
    parcelable ComponentTraits {
        @VintfStability
        @Backing(type="int")
        enum Kind {
            OTHER = 0,
            DECODER,
            ENCODER,
        }
        @VintfStability
        @Backing(type="int")
        enum Domain {
            OTHER = 0,
            VIDEO,
            AUDIO,
            IMAGE,
        }
        /**
         * Name of the component. This must be unique for each component.
         *
         * This name is use to identify the component to create in
         * createComponent() and createComponentInterface().
         */
        String name;
        /**
         * Component domain.
         */
        Domain domain;
        /**
         * Component kind.
         */
        Kind kind;
        /**
         * Rank used by `MediaCodecList` to determine component ordering. Lower
         * value means higher priority.
         */
        int rank;
        /**
         * MIME type.
         */
        String mediaType;
        /**
         * Aliases for component name for backward compatibility.
         *
         * Multiple components can have the same alias (but not the same
         * component name) as long as their media types differ.
         */
        String[] aliases;
    }
    // Ignoring method createComponent_1_1 from 1.1::IComponentStore since a newer alternative is available.
    // Ignoring method createComponent from 1.0::IComponentStore since a newer alternative is available.

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param Status status since there is only one return value.
    /**
     * Copies the contents of @p src into @p dst without changing the format of
     * @p dst.
     *
     * @param src Source buffer.
     * @param dst Destination buffer.
     * @return Status of the call, which may be
     *   - `OK`        - The copy is successful.
     *   - `CANNOT_DO` - @p src and @p dst are not compatible.
     *   - `REFUSED`   - No permission to copy.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     */
    Status copyBuffer(in Buffer src, in Buffer dst);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Changing method name from createComponent_1_2 to createComponent
    /**
     * Creates a component by name.
     *
     * @param name Name of the component to create. This must match one of the
     *     names returned by listComponents().
     * @param listener Callback receiver.
     * @param pool `IClientManager` object of the BufferPool in the client
     *     process. This may be null if the client does not own a BufferPool.
     * @param out status Status of the call, which may be
     *   - `OK`        - The component was created successfully.
     *   - `NOT_FOUND` - There is no component with the given name.
     *   - `NO_MEMORY` - Not enough memory to create the component.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out comp The created component if @p status is `OK`.
     *
     * @sa IComponentListener.
     */
    void createComponent(in String name, in IComponentListener listener,
        in android.hardware.media.bufferpool2.IClientManager pool,
        out Status status, out IComponent comp);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Creates a persistent input surface that can be used as an input surface
     * for any IComponent instance
     *
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation was successful.
     *   - `NO_MEMORY` - Not enough memory to complete this method.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out surface A persistent input surface. This may be null to indicate
     *     an error.
     */
    void createInputSurface(out Status status, out IInputSurface surface);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Creates a component interface by name.
     *
     * @param name Name of the component interface to create. This should match
     *     one of the names returned by listComponents().
     * @param out status Status of the call, which may be
     *   - `OK`        - The component interface was created successfully.
     *   - `NOT_FOUND` - There is no component interface with the given name.
     *   - `NO_MEMORY` - Not enough memory to create the component interface.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out compIntf The created component interface if @p status is `OK`.
     */
    void createInterface(in String name, out Status status, out IComponentInterface compIntf);

    // Adding return type to method instead of out param IConfigurable configurable since there is only one return value.
    /**
     * Returns the @ref IConfigurable instance associated to this component
     * store.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();

    // Adding return type to method instead of out param android.hardware.media.bufferpool2.IClientManager pool since there is only one return value.
    /**
     * Returns the `IClientManager` object for the component's BufferPool.
     *
     * @return If the component store supports receiving buffers via
     *     BufferPool API, @p pool must be a valid `IClientManager` instance.
     *     Otherwise, @p pool must be null.
     */
    android.hardware.media.bufferpool2.IClientManager getPoolClientManager();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Returns a list of `StructDescriptor` objects for a set of requested
     * C2Param structure indices that this store is aware of.
     *
     * This operation must be performed at best effort, e.g. the component
     * store must simply ignore all struct indices that it is not aware of.
     *
     * @param indices Indices of C2Param structures to describe.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `NOT_FOUND` - Some indices were not known.
     *   - `NO_MEMORY` - Not enough memory to complete this method.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out structs List of `StructDescriptor` objects.
     */
    void getStructDescriptors(in int[] indices,
        out Status status, out StructDescriptor[] structs);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Returns the list of components supported by this component store.
     *
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation was successful.
     *   - `NO_MEMORY` - Not enough memory to complete this method.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out traits List of component traits for all components supported by
     *     this store (in no particular order).
     */
    void listComponents(out Status status, out ComponentTraits[] traits);
}
