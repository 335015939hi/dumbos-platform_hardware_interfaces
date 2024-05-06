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

import android.hardware.security.see.storage.AllowedTamper;
import android.hardware.security.see.storage.Filesystem;

parcelable SessionOptions {
    /**
     * Properties of the filesystem to create a session for.
     */
    Filesystem filesystem;

    /**
     * Set to acknowledge possible files tampering.
     *
     * If unacknowledged tampering is detected, operations will fail with an `ERR_FS_*`
     * service-specific code.
     */
    AllowedTamper integrity = AllowedTamper.NO_TAMPER;

    /**
     * Allow writes to succeed while the filesystem is in the middle of an A/B update.
     *
     * If the A/B update fails, writes which were made during the upodate will be rolled back. This
     * rollback will _not_ be reported as tampering.
     *
     * With this value set to false (recommended), operations attempted during an A/B update will
     * fail with the `ERR_AB_UPDATE_IN_PROGRESS` service-specific code.
     */
    boolean allowWritesDuringAbUpdate = false;
}
