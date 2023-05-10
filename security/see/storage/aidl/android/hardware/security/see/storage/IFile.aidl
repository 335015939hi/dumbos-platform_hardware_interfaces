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

/** The interface for an open file */
interface IFile {
    /** Read bytes from this file. Reads bytes [offset, offset + size). */
    byte[] Read(long size, long offset);

    /**
     * Write the bytes in `buffer` this file, starting at `offset`.
     *
     * Returns the number of bytes written successfully.
     */
    long Write(long offset, in byte[] buffer);

    long GetSize();

    void SetSize(long new_size);

    /** Close the file. */
    void Close();
}
