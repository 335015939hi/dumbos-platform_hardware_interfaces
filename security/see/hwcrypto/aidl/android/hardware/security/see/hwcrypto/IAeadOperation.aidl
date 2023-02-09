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

/*
 * interface IEmittingOperation - Interface for a authenticated encryption with additional data
 *                                 operation that generates an output on each invocation.
 *
 */
interface IAeadOperation {
    /*
     * update_aad() - Update additional data.  Implementations can assume that all calls to
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
    byte[] update(in byte[] data);

    /*
     * finish() - Complete operation. It takes data to not require an extra trip to finalize the
     *            operation.
     * @data:
     *      data to be processed by the cryptographic operation
     *
     * Return:
     *      Ok(Byte Vector) on success, specific error code on error.
     */
    byte[] finish(in byte[] data);
}
