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

import android.hardware.security.see.hwcrypto.base_types.EvictReason;
import android.hardware.security.see.hwcrypto.base_types.KeyLifetime;
import android.hardware.security.see.hwcrypto.base_types.KeyPermissions;
import android.hardware.security.see.hwcrypto.base_types.KeyType;
import android.hardware.security.see.hwcrypto.base_types.KeyUse;

/*
 * parcelable KeyPolicy - Structure that specified how a key can be used.
 *
 * @usage:
 *      Enum specifying the operations the key can perform (encryption, decryption, etc.).
 * @key_class:
 *      Enum that describes the key type. See the docstring on <code>KeyLifetime</code> for more
 *      details.
 * @evict_policy:
 *      Policy to invalidate the key. See the docstring on <code>EvictReason</code> for more
 *      details.
 * @key_permissions:
 *      Additional permissions of the key (e.g. key types allowed to wrap the key, boot binding,
 *      etc.). See the docstring on <code>KeyPermissions</code> for more details.
 */
parcelable KeyPolicy {
    KeyUse usage = KeyUse.UNSPECIFIED; // TODO: check if using a vector would be preferable
    KeyLifetime key_class = KeyLifetime.EPHEMERAL;
    // TODO: how much can this evict policy be enforced if you have a wrapped copy of the key
    //       (Define the expectations/assumptions).
    EvictReason[] evict_policy;
    KeyPermissions[] key_permissions;
    KeyType key_type = KeyType.AES_256_GCM;
}
