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

/**
 * Interface for a single Secure Storage session
 *
 * When the session is opened, it will start a transaction and any changes made through this session
 * or the interfaces this session returns will be added to this transaction's pending changes.
 * Calling `CommitChanges`/`AbandonChanges` will commit/abandon these pending changes, and start a
 * new, empty transaction. The interfaces this session returns _remain_ valid across transactions;
 * it is not necessary, for example, to reopen a file after a commit.
 *
 * Any changes still pending when the session is dropped will be abandoned.
 */
interface ISession {
    const int ERR_UNSUPPORTED_PROPERTIES = 1;
    const int ERR_NOT_FOUND = 2;
    const int ERR_ALREADY_EXISTS = 3;
    const int ERR_BAD_TRANSACTION = 4;

    /**
     * Commits any pending changes made through this session to storage.
     *
     * The session will no longer have pending changes after this call returns. Files may then still
     * be modified through this session to create another commit.
     *
     * May return service-specific errors:
     *   - ERR_BAD_TRANSACTION
     */
    void CommitChanges();
    /**
     * Abandons any pending changes made through this session.
     */
    void AbandonChanges();

    /**
     * Opens a secure file for writing and/or reading.
     *
     * Changes made to the file are part of the current transaction. Dropping this session
     * invalidates the returned `IFile` interface
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
     * This method cannot change the properties of an existing file, only its name
     * The provided `fileProperties` are only used to identify the source file;
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS
     */
    void RenameFile(in String currentName, in FileProperties fileProperties, in String newName);

    /**
     * Opens a directory from a filesystem with the given properties.
     *
     *  Dropping this session invalidates the returned `IDir` interface.
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    OpenDirResult OpenDir(in String path, in FileProperties fileProperties);
}
