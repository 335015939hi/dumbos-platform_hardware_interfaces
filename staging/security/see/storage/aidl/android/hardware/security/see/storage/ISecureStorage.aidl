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

import android.hardware.security.see.storage.DeleteOptions;
import android.hardware.security.see.storage.FileProperties;
import android.hardware.security.see.storage.IDir;
import android.hardware.security.see.storage.IFile;
import android.hardware.security.see.storage.OpenOptions;
import android.hardware.security.see.storage.ReadIntegrity;
import android.hardware.security.see.storage.RenameOptions;

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

    const int ERR_FS_RESET = 5;
    const int ERR_FS_ROLLED_BACK = 6;
    const int ERR_FS_TAMPERED = 7;

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
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    IFile openFile(in @utf8InCpp String filePath, in FileProperties fileProperties,
            in OpenOptions options);

    /**
     * Delete a file.
     *
     * @filePath:
     *     path to the file, relative to filesystem root
     * @fileProperties:
     *     properties originally requested for the file being deleted
     * @options:
     *     options controlling deletion behavior
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    void deleteFile(in @utf8InCpp String filePath, in FileProperties fileProperties,
            in DeleteOptions options);

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
     *   - ERR_NOT_FOUND if no file exists at @currentPath, or if @options.destCreateMode is
     *       `NO_CREATE` and no file exists at @destPath
     *   - ERR_ALREADY_EXISTS if @options.destCreateMode is `CREATE_EXCLUSIVE` and a file exists at
     *       @destPath
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @options.readIntegrity
     *       does not acknowledge
     */
    void renameFile(in @utf8InCpp String currentPath, in FileProperties fileProperties,
            in @utf8InCpp String destPath, in RenameOptions options);

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
     *     allow opening (and subsequent read/write operations) despite possible tampering for the
     * directory
     *
     * May return service-specific errors:
     *   - ERR_NOT_FOUND
     *   - ERR_UNSUPPORTED_PROPERTIES
     *   - ERR_FS_* if the filesystem has been tampered with in a way that @readIntegrity does not
     *       acknowledge
     */
    IDir openDir(in @utf8InCpp String path, in FileProperties fileProperties,
            in ReadIntegrity readIntegrity);
}
