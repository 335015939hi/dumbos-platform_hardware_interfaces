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
package android.hardware.security.see.block;

interface IBlockDevice {
    /** Get the number of memory blocks this device controls */
    long blockCount();

    /**
     * Read from this device.
     *
     * @startBlock:
     *     The block at which to start reading
     * @numBlocks:
     *     The total number of blocks to read
     *
     * Return:
     *     the bytes contained by the blocks [@startBlock, @startBlock + @numBlocks)
     */
    byte[] read(long startBlock, long numBlocks);
    /**
     * Writes to this device.
     *
     * @startBlock:
     *     The block at which to start writing
     * @data:
     *     The contents to write. Length must be a multiple of this device's block size.
     */
    void write(long startBlock, in byte[] data);
}
