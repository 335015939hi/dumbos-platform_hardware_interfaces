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

import android.hardware.security.see.hwcrypto.types.DataType;
import android.hardware.security.see.hwcrypto.types.EcSignParameters;
import android.hardware.security.see.hwcrypto.types.HmacOperationParameters;
import android.hardware.security.see.hwcrypto.types.RsaDecryptParameters;
import android.hardware.security.see.hwcrypto.types.RsaSignParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricAuthOperationParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricOperationParameters;

/*
 * parcelable MemoryBufferParameter - Structure representing a memory buffer.
 *
 * @buffer_handle:
 *      Handle used to access this memory area.
 * @size:
 *      Total size of the memory buffer.
 */
parcelable MemoryBufferParameter {
    DataType buffer_type;
    ParcelFileDescriptor buffer_handle;
    int size;
}
