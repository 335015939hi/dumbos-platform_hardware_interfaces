package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;
import android.hardware.security.see.hwcrypto.base_types.U64Result;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

/*
 * interface IAccumulatingOperation - Interface for a cryptographic operation that only generates a
 *                                    result on the last invocation.
 *
 */
interface IAccumulatingOperation {
    /*
     * max_input_size() - Maximum size of accumulated input.
     *
     * Return:
     *      Ok(long) on success, specific error code on error.
     */
    U64Result max_input_size();

    /*
     * update() - Update operation with data.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode update(in byte[] data);

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
