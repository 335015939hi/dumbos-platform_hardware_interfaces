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

/** The interface for an open directory */
interface IDir {
    /**
     * Gets the next batch of filenames in this directory.
     *
     * Calling multiple times will return different results as the IDir iterates through all the
     * files it contains. When all filenames have been returned, all successive calls will return an
     * empty list.
     *
     * Returns:
     *     An ordered list of filenames.
     */
    String[] readNextFilenames();
}
