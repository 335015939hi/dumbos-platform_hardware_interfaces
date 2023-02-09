package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

/*
 * interface IEmittingOperation - Interface for a authenticated encryption with aditional data
 *                                 operation that generates an output on each invocation.
 *
 */
interface IAeadOperation {
    /*
     * update() - Update additional data.  Implementations can assume that all calls to
     *            `update_aad()` will occur before any calls to `update()` or `finish()`.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode update_aad(in byte[] aad);

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
