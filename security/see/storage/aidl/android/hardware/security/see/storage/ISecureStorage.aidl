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

import android.hardware.security.see.storage.ISession;

/** Interface for the Secure Storage HAL */
interface ISecureStorage {
    const int ERR_UNSUPPORTED_PROPERTIES = 1;

    /**
     * Starts a storage session.
     *
     * @multifileAtomicCommits: If true, the resulting session will allow changes to
     *     multiple file to be part of a single commit and guarantees that these changes
     *     will suceed or fail atomically. This function will return
     *     ERR_UNSUPPORTED_PROPERTIES if the filesystem does not support multi-file
     *     atomic commits.
     *
     * May return service-specific errors:
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    ISession StartSession(boolean multifileAtomicCommits);
}
