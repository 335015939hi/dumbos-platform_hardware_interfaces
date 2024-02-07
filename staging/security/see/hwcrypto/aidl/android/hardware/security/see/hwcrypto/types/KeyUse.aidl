/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto.types;

/*
 * Enum describing the allowed operations that can be performed with the given key.
 */
@Backing(type="int")
enum KeyUse {
    ENCRYPT = 1,
    DECRYPT = 2,
    ENCRYPT_DECRYPT = ENCRYPT | DECRYPT,
    SIGN = 4,
    VERIFY = 8,
    SIGN_VERIFY = SIGN | VERIFY,
    EXCHANGE = 16,
    DERIVE = 32,
    EXCHANGE_DERIVE = EXCHANGE | DERIVE,
    WRAP = 64,
    /* combination of all uses*/
    ANY = 127,
}
