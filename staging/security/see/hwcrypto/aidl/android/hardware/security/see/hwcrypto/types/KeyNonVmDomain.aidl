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
 * Enum that represents the different non-VM domains that can own or be granted permissions to use
 * a key.
 */
enum KeyNonVmDomain {
    /*
     * Domain has a security level comparable to an SoC Hardware Root of Trust IP block.
     */
    ROOT_OF_TRUST = 1,
    /*
     * Domain has a security level comparable to a Security Processor running separatedly from the
     * main application processor.
     */
    SECURE_ENCLAVE = 2,
    /*
     * Domain has a security level comparable to a Trust Zone application.
     */
    TRUSTED_ENVIRONMENT = 4,
    /*
     * Domain is the Android Host OS.
     */
    HOST = 8,
}
