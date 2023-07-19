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
package android.hardware.security.see.dma;

import android.hardware.security.see.dma.DmaBufferArea;
import android.hardware.security.see.dma.DmaBufferType;

interface IDmaOperations {
    /*
     * create_buffer_area() - Creates a DMA area.
     *
     * @buffer:
     *      file descriptor to access the DMA buffer
     * @buffer_length:
     *      length of the DMA buffer
     * @buffer_type:
     *      type of shared buffer. See <code>DmaBufferType</code> for more details
     *
     * Return:
     *      DmaBufferArea on success
     */
    DmaBufferArea create_buffer_area(
            in ParcelFileDescriptor buffer, long buffer_length, DmaBufferType buffer_type);

    void destroy_buffer_area(in DmaBufferArea buffer);
}
