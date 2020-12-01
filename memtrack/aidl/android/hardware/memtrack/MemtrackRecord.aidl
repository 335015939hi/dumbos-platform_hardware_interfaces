/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.memtrack;

/*
 * A vector of MemtrackRecord is returned by the function getMemory().
 * Each record consists of the size of the memory used by the process and
 * flags indicate all the MemtrackFlags that are valid for this record.
 * see getMemory() comments for further details.
 */
@VintfStability
parcelable MemtrackRecord {
    long sizeInBytes;
    /**
     * This is the bitfield for the MemtrackFlag indicating all the flags that
     * are valid for this record.
     */
    int flags;
}

