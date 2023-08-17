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
import android.hardware.security.see.hwcrypto.types.ScatterGatherLists;

interface IDmaAeadOperation {
    /*
     * update() - Call used to update operation with data provided by DMA input buffer.
     *
     * @data:
     *      data to be processed by the cryptographic operation. AAD is included here, but if any
     *      encrypted buffer has been processed, no additional AAD can be sent.
     * @wait_for_completion:
     *      If true operation will not return until it has been finished. If false, call will
     *      immediately return if possible.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode update(in ScatterGatherLists data, boolean wait_for_completion);

    /*
     * is_busy() - Non-Blocking call used to poll the status of the current
     *             cryptographic operation to check if it has finished.
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
     * finish() - Call used to complete operation. After this function is called, no
     *            further operations are allowed on this interface.
     * @data:
     *      data to be processed by the cryptographic operation
     * @wait_for_completion:
     *      If true operation will not return until it has been finished. If false, call will
     *      immediately return if possible.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode finish(in ScatterGatherLists data, boolean wait_for_completion);
}
