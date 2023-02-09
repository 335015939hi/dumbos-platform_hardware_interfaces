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
    long max_input_size();

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
    byte[] finish(in byte[] data);
}
