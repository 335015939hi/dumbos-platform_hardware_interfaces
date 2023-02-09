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

// Lifetime values represented as a bitmask. This allow us to combined them on a single property on
// operations to describe a set of allowed lifetimes. Enum values set to match key permissions.
/*
 * enum KeyLifetime - Gives more information about the lifetime characteristics of the key.
 *
 * @EPHEMERAL:
 *      Hardware keys with limited validity (until key is erased or power cycle occurs).
 * @HARDWARE:
 *      Key only lives or was derived from a key that only lives in hardware. This key cannot be
 *      retrieved in the clear.
 * @PORTABLE:
 *      Key could have been at some point of its lifetime in the clear on a software component.
 */
@Backing(type="byte")
enum KeyLifetime {
    EPHEMERAL = 2,
    HARDWARE = 4,
    PORTABLE = 8,
}
