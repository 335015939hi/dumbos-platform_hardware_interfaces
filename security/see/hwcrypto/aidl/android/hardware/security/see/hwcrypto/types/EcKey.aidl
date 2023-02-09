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
package android.hardware.security.see.hwcrypto.types;

import android.hardware.security.see.hwcrypto.types.Ed25519Key;
import android.hardware.security.see.hwcrypto.types.NistKey;
import android.hardware.security.see.hwcrypto.types.X25519Key;

union EcKey {
    NistKey P224;
    NistKey P256;
    NistKey P384;
    NistKey P521;
    Ed25519Key Ed25519;
    X25519Key X25519;
}
