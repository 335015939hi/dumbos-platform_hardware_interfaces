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
package android.hardware.security.see.storage;

enum AllowedTamper {
    /**
     * Return an error if any REE alteration of the written data has been detected.
     *
     * (Full resets of non-persistent filesystems, like factory resets, are not considered
     * tampering and are thus never reported.)
     */
    NONE,

    /**
     * Ignores REE tampering which has reset a file/filesystem to its initial state. Returns a
     * tampering error for any other alterations.
     */
    IGNORE_RESET,

    /**
     * Ignores REE tampering which has rolled a file/filesystem back to a valid checkpoint. Returns
     * a tampering error for any other alterations.
     *
     * What makes a checkpoint valid is implementation defined; an implementation might take a
     * checkpoint on its first post-factory boot. This option is strictly more permissive than
     * `IGNORE_RESET`. (In other words, the initial state is always a valid checkpoint.)
     */
    IGNORE_ROLLBACK,

    // There's no `IGNORE_ALL` because if REE has done any alteration other
    // than a rollback, the file contents will be known-bad data.
}
