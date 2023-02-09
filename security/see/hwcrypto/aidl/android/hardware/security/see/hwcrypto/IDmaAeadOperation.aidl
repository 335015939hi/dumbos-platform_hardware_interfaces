package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.BooleanResult;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

interface IDmaAeadOperation {
    /*
     * update() - Update additional data.  Implementations can assume that all calls to
     *            `update_aad()` will occur before any calls to `update()` or `finish()`.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode update_aad();

    // Update operation with data.
    /*
     * finish() - Non-Blocking call used to update operation with data provided by DMA input buffer.
     *            Operation result will be written to output DMA buffer.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode update();

    /*
     * wait_for_completion() - Non-Blocking call used to poll the status of the current
     *                         cryptographic operation to check if it has finished.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    BooleanResult is_busy();

    /*
     * wait_for_completion() - Blocking call that will not return until the current cryptographic
     *                         operation has finished.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode wait_for_completion();

    /*
     * finish() - Non-Blocking call used to complete operation. It will try to do a last call using
     *            the DMA buffers.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode finish();

    /*
     * abort() - Aborts the operation.
     *
     * Return:
     *      Nothing.
     */
    void abort();

    // TODO: See if we should also add a callback type function call.
}
