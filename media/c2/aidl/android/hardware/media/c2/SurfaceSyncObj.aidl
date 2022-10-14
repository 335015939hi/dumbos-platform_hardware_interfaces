// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Surface(BufferQueue/IGBP) synchronization object regarding # of dequeued
 * output buffers. This keeps # of dequeued buffers from Surface less than
 * configured max # of dequeued buffers all the time.
 */
@VintfStability
parcelable SurfaceSyncObj {
    /**
     * ASharedMemory for synchronization data. Layout is below
     *
     * |lock(futex)                               4bytes|
     * |conditional_variable(futex)               4bytes|
     * |# of max dequeable buffer                 4bytes|
     * |# of dequeued buffer                      4bytes|
     * |Status of the surface                     4bytes|
     *      INIT        = 0, Configuring surface is not finished.
     *      ACTIVE      = 1, Surface is ready to allocate(dequeue).
     *      SWITCHING   = 2, Switching to the new surface. It is blocked
     *                       to allocate(dequeue) a buffer until switching
     *                       completes.
     */
    android.os.NativeHandle syncMemory;
    /**
     * BufferQueue id.
     */
    long bqId;
    /**
     * Generation id.
     */
    int generationId;
    /**
     * Consumer usage flags. See +ndk
     * libnativewindow#AHardwareBuffer_UsageFlags for possible values.
     */
    long consumerUsage;
}
