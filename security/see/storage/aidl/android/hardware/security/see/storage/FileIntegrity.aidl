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
package android.hardware.security.see.storage;

enum FileIntegrity {
    // A TAMPER_PROOF option requiring that REE can neither prevent nor tamper
    // with file operations can be added if/when and OEM supports it.
    /** REE may prevent operations, but cannot alter data once written. */
    TAMPER_PROOF_AT_REST,
    /**
     * REE may alter written data, but changes will be detected and reported as
     * an error on read.
     */
    TAMPER_DETECT,
    /**
     * REE may alter written data. Changes other than resets will be detected
     * and reported.
     */
    TAMPER_DETECT_IGNORE_RESET,
    /**
     * REE may alter written data. Changes will be detected and reported, unless
     * those changes are rollbacks that occurred because a write was accepted
     * during an A/B update, then a boot failure caused a rollback to a checkpoint.
     */
    TAMPER_DETECT_IGNORE_ROLLBACK_AB_BOOT_FAILURE,
}
