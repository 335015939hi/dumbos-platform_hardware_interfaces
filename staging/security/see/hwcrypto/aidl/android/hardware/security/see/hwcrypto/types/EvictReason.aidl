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
 * Enum that defines on which case the key should be invalidated. Notice that hardware support might
 * be needed to provide these guarantees.
 */
@Backing(type="byte")
enum EvictReason {
    /*
     * Keys should be invalidated if the security state of the Security anchor (e.g. GSA) changes.
     */
    SECURITY_ANCHOR_STATE_CHANGE = 1,
    /*
     * Keys should be invalidated if the security state of the secure enclave (e.g. TZ) changes.
     */
    SECURE_ENCLAVE_STATE_CHANGE = 2,
    /*
     * Keys should be invalidated if the security state of the non-secure world changes.
     */
    NON_SECURE_WORLD_STATE_CHANGE = 4,
}
