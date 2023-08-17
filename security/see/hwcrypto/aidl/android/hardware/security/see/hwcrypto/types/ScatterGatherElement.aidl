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
package android.hardware.security.see.hwcrypto.types;

import android.hardware.security.see.hwcrypto.types.NullableScatterGatherElementType;

/*
 * parcelable ScatterGatherElement - Structure representing a single memory buffer.
 *
 * @type:
 *      Element type. See <code>ScatterGatherElementType</code> for more details. Notice that this
 *      field only has meaning for encryption inputs and if it is omitted on that case, it is
 *      assumed that this element is an encrypted buffer.
 * @buffer_handle:
 *      Handle used to access this memory area.
 * @start_offset:
 *      Start of the scatter/gather buffer measured from the start of the memory area pointed by
 *      <code>buffer_handle</code>.
 * @size:
 *      Total size of the scatter/gather buffer. Implementation shall check that both
 *      <code>start_offset</code> and <code>start_offset</code>+<code>size</code> fall inside the
 *      memory area pointed by <code>buffer_handle</code>.
 */
parcelable ScatterGatherElement {
    @nullable NullableScatterGatherElementType type;
    ParcelFileDescriptor buffer_handle;
    int start_offset;
    int size;
}
