package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.VectorResult;

/*
 * interface IEmittingOperation - Interface for a cryptographic operation that generates an output
 *                                on each invocation.
 *
 */
interface IEmittingOperation {
    /*
     * update() - Update operation with data.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      Ok(Byte Vector) on success, specific error code on error.
     */
    VectorResult update(in byte[] data);

    /*
     * finish() - Complete operation. It takes data to not require an extra trip to finalize the
     *            operation.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      Ok(Byte Vector) on success, specific error code on error.
     */
    VectorResult finish(in byte[] data);

    /*
     * abort() - Aborts the operation.
     *
     * Return:
     *      Nothing.
     */
    void abort();
}
