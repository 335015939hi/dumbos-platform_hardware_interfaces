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

import android.hardware.security.see.storage.FileProperties;
import android.hardware.security.see.storage.OpenOptions;
import android.hardware.security.see.storage.result.OpenDirResult;
import android.hardware.security.see.storage.result.OpenFileResult;

/** Interface for a single Secure Storage session */
interface ISession {
    const int ERR_UNSUPPORTED_PROPERTIES = 1;
    const int ERR_NOT_FOUND = 2;
    const int ERR_ALREADY_EXISTS = 3;
    const int ERR_BAD_TRANSACTION = 4;

    /**
     * Commits any pending changes made through this session to storage.
     */
    void CommitChanges();
    /**
     * Abandons any pending changes made through this session.
     */
    void AbandonChanges();
    /**
     * End this active session.
     *
     * @commit: If true, commits any pending changes before closing. Otherwise, abandons them.
     */
    void Close(boolean commit);

    /**
     * Opens a secure file for writing and/or reading.
     *
     * Changes made to the file are part of the current transaction.
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    OpenFileResult OpenFile(
            in String fileName, in FileProperties fileProperties, in OpenOptions options);

    /**
     * Delete a file.
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    void DeleteFile(in String fileName, in FileProperties fileProperties);

    /**
     * Renames an existing file.
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS
     */
    void RenameFile(in String currentName, in FileProperties fileProperties, in String newName);

    /**
     * Opens a directory from a filesystem with the given properties.
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    OpenDirResult OpenDir(in String path, in FileProperties fileProperties);
}
