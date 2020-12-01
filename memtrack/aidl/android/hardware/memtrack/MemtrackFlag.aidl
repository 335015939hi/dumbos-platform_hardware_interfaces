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

/**
 * SMAPS_ACCOUNTED/SMAPS_UNACCOUNTED
 * Flags to differentiate memory that can already be accounted for in
 * /proc/<pid>/smaps,
 * (Shared_Clean + Shared_Dirty + Private_Clean + Private_Dirty = Size).
 * In general, memory mapped in to a userspace process is accounted unless
 * it was mapped with remap_pfn_range.
 * Exactly one of these must be set.
 *
 * SHARED/SHARED_PSS/PRIVATE
 * Flags to differentiate memory shared across multiple processes vs. memory
 * used by a single process.
 * If SHARED_PSS flags is used, the memory must be divided by the number of
 * processes holding reference to it (shared / num_processes).
 * Only zero or one of these may be set in a record.
 * If none are set, record is assumed to count shared + private memory.
 *
 * SYSTEM/DEDICATED
 * Flags to differentiate memory taken from the kernel's allocation pool vs.
 * memory that is dedicated to non-kernel allocations, for example a carveout
 * or separate video memory.  Only zero or one of these may be set in a record.
 * If none are set, record is assumed to count system + dedicated memory.
 *
 * NONSECURE/SECURE
 * Flags to differentiate memory accessible by the CPU in non-secure mode vs.
 * memory that is protected.  Only zero or one of these may be set in a record.
 * If none are set, record is assumed to count secure + nonsecure memory.
 */
@VintfStability
@Backing(type="int")
enum MemtrackFlag {
    SMAPS_ACCOUNTED = 1 << 1,
    SMAPS_UNACCOUNTED = 1 << 2,
    SHARED = 1 << 3,
    SHARED_PSS = 1 << 4,
    PRIVATE = 1 << 5,
    SYSTEM = 1 << 6,
    DEDICATED = 1 << 7,
    NONSECURE = 1 << 8,
    SECURE = 1 << 9,
}
