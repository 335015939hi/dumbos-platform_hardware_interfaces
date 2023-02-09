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

import android.hardware.security.see.hwcrypto.types.EcSignParameters;
import android.hardware.security.see.hwcrypto.types.HmacOperationParameters;
import android.hardware.security.see.hwcrypto.types.RsaDecryptParameters;
import android.hardware.security.see.hwcrypto.types.RsaSignParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricAuthOperationParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricOperationParameters;

/*
 * union OperationParameters - Type that describes the parameters for the different operations that
 *                             can be performed.
 *
 * @SymmetricAuthCrypto:
 *      Parameters for authenticated symmetric cryptography (AES GCM).
 * @SymmetricCrypto:
 *      Enum that describes the key lifetime characteristics. See the docstring on
 *      <code>KeyLifetime</code> for more details.
 * @Hmac:
 *      Policy to invalidate the key. See the docstring on <code>EvictReason</code> for more
 *      details.
 * @EcSign:
 *      Additional permissions of the key (e.g. key types allowed to wrap the key, boot binding,
 *      etc.). See the docstring on <code>KeyPermissions</code> for more details.
 * @RsaDecrypt:
 *      Enum that specifies the key type.
 * @RsaSign:
 *      Domain that owns this key. It is used for decisions like if caller is allowed to wrap a
 *      key.
 */
union OperationParameters {
    SymmetricAuthOperationParameters SymmetricAuthCrypto;
    SymmetricOperationParameters SymmetricCrypto;
    HmacOperationParameters Hmac;
    EcSignParameters EcSign;
    RsaDecryptParameters RsaDecrypt;
    RsaSignParameters RsaSign;
}
