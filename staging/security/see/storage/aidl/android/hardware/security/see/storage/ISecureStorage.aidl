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
import android.hardware.security.see.storage.ReadIntegrity;
import android.hardware.security.see.storage.RenameOptions;
import android.hardware.security.see.storage.result.OpenDirResult;
import android.hardware.security.see.storage.result.OpenFileResult;

/**
 * Interface for the Secure Storage HAL
 *
 * When the connection is opened, it will start a transaction and any changes made through this
 * session or the interfaces this session returns will be added to this transaction's pending
 * changes. Calling `CommitChanges`/`AbandonChanges` will commit/abandon these pending changes, and
 * start a new, empty transaction. The interfaces this session returns _remain_ valid across
 * transactions; it is not necessary, for example, to reopen a file after a commit.
 *
 * Any changes still pending when the connection is dropped will be abandoned.
 */
interface ISecureStorage {
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
    void commitChanges();
    /**
     * Abandons any pending changes made through this session.
     */
    void abandonChanges();

    /**
     * Starts an sequence of changes to be applied atomically.
     *
     * Any changes made through this session after the call to `StartAtomicSegment` will be
     * applied as one atomic unit. Calling `EndAtomicSegment` will end the segment, allowing
     * more non-atomic changes to be added to the same commit. Committing or abandoning pending
     * changes will also end the atomic segment.
     *
     * Some implementations may not support atomic segments containing changes to multiple files.
     * The file-modifying methods of `IFile` or  `ISecureStorageISecureStorage` will  return
     * `ERR_UNSUPPORTED_PROPERTIES` if a second file is attempted to be modified and the
     * implementation doesn't support multi-file atomics.
     */
    void startAtomicSegment();
    /**
     * Ends the current atomic sequence.
     *
     * Allows subsecquent changes (and/or atomic segments) to be added to the same commit as
     * the ended atomic segment.
     */
    void endAtomicSegment();

    /**
     * Opens a secure file for writing and/or reading.
     *
     * Changes made to the file are part of the current transaction. Dropping this session
     * invalidates the returned `IFile` interface
     *
     * @filePath:
     *     path to the file, relative to filesystem root
     * @fileProperties:
     *     properties requested for this file
     * @options:
     *     options controlling opening behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    OpenFileResult openFile(
            in String filePath, in FileProperties fileProperties, in OpenOptions options);

    /**
     * Delete a file.
     *
     * @filePath:
     *     path to the file, relative to filesystem root
     * @fileProperties:
     *     properties originally requested for the file to be deleted
     * @readIntegrity:
     *     set to delete despite possible tampering for the file
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES if this the second file modified as part of an atomic segment
     *       on a filesystem that does not support multi-file atomics.
     */
    void deleteFile(
            in String filePath, in FileProperties fileProperties, in ReadIntegrity readIntegrity);

    /**
     * Renames an existing file.
     *
     * This method cannot change the properties of an existing file, only its name.
     * The provided `fileProperties` are only used to identify the source file;
     * file properties cannot be changed after creation.
     * The file must not already be opened.
     *
     * @currentPath:
     *     path to the file, relative to filesystem root
     * @fileProperties:
     *     properties originally requested for the file
     * @destPath:
     *     the file's new path, relative to filesystem root
     * @options:
     *     options controlling rename behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_ALREADY_EXISTS if a file already exists at @destPath and @options.overwriteDest is
     *       false
     *   - ERR_UNSUPPORTED_PROPERTIES if this the second file modified as part of an atomic segment
     *       on a filesystem that does not support multi-file atomics.
     */
    void renameFile(in String currentPath, in FileProperties fileProperties, in String destPath,
            in RenameOptions options);

    /**
     * Opens a directory from a filesystem with the given properties.
     *
     * Dropping this session invalidates the returned `IDir` interface.
     *
     * @path:
     *     path to the directory, relative to filesystem root
     * @fileProperties:
     *     properties of the files in this directory
     * @readIntegrity:
     *     set to read despite possible tampering for the directory
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES
     */
    OpenDirResult openDir(
            in String path, in FileProperties fileProperties, in ReadIntegrity readIntegrity);
}
