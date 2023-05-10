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

import android.hardware.security.see.storage.RenameOptions;

/** The interface for an open file */
interface IFile {
    /**
     * Read bytes from this file.
     *
     * @size:
     *     the size (in bytes) of the segment to read
     * @offset:
     *     the offset (in bytes) at which to start reading
     *
     * Return:
     *     the sequence of bytes at [offset, offset + size) in the file
     */
    byte[] read(long size, long offsetf);

    /**
     * Write the bytes in `buffer` to this file.
     *
     * @offset:
     *     the offset (in bytes) at which to start writing
     *
     * Return:
     *     the number of bytes written successfully
     *
     * May return service-specific errors:
     *   - ERR_UNSUPPORTED_PROPERTIES if this the second file modified as part of an atomic segment
     *       on a filesystem that does not support multi-file atomics.
     */
    long write(long offset, in byte[] buffer);

    /** Reads this file's size. */
    long getSize();

    /**
     * Sets this file's size.
     *
     * Truncates the file if `new_size` is less than the current size.
     *
     * @newSize:
     *     the file's new size
     *
     * May return service-specific errors:
     *   - ERR_UNSUPPORTED_PROPERTIES if this the second file modified as part of an atomic segment
     *       on a filesystem that does not support multi-file atomics.
     */
    void setSize(long newSize);

    /**
     * Renames this file.
     *
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
    void renameFile(in String destPath, in RenameOptions options);
}
