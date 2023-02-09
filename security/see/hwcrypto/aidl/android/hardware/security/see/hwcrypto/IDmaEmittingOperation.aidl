/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.types.HalErrorCode;

interface IDmaEmittingOperation {
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
    boolean is_busy();

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
