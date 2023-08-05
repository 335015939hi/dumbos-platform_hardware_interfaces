/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.secretkeeper;

@VintfStability
/**
 * Secretkeeper service definition.
 *
 * An ISecretkeeper instance provides secure storage of secrets on behalf of other components in
 * Android, in particular for protected virtual machine instances. As such, the implementation
 * of ISecretkeeper should live in a secure environment, such as:
 * - A trusted execution environment such as ARM TrustZone.
 * - A completely separate, purpose-built and certified secure CPU.
 *
 * TODO(b/291224769): Extend the HAL interface to include:
 * 1. Session setup api: This is used to perform cryptographic operations that allow shared keys be
 * agreed between session participants, typically (but not necessarily) a pVM instance and
 * Secretkeeper. This session setup is based on public key information.
 * 2. Dice policy operation - These allow sealing of the secrets with a class of Dice chains.
 * Typical operations are (securely) updating the dice policy sealing the Secrets above. These
 * operations are core to AntiRollback protected secrets - ie, ensuring secrets of a pVM are only
 * accessible to same or higher versions of the images.
 * 3. Maintenance api: This is required for removing the Secretkeeper entries for obsolete pvMs.
 */
interface ISecretkeeper {
    /**
     * Store the client's secret.
     */
    void store(in byte[] payload);
    /**
     * Retrieve the client's secret.
     */
    byte[] read(in byte[] payload);
}
