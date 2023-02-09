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

import android.hardware.security.see.hwcrypto.types.OperationData;

/*
 * parcelable PatternParameters - Structure that specifies a pattern to process data.
 *
 * @number_blocks_process:
 *      Number of blocks that will be processed. The size of the block matches the size of the
 *      cipher used.
 * @number_blocks_copy:
 *      Number of blocks that will be copied. The size of the block matches the size of the cipher
 *      used to process the encrypted areas.
 */
parcelable PatternParameters {
    long number_blocks_process;
    long number_blocks_copy;
}
