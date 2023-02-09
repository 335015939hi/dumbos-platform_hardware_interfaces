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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.CopyParameters;
import android.hardware.security.see.hwcrypto.FinishParameters;
import android.hardware.security.see.hwcrypto.MemoryBufferParameter;
import android.hardware.security.see.hwcrypto.OperationParameters;
import android.hardware.security.see.hwcrypto.types.EmptyEnum;
import android.hardware.security.see.hwcrypto.types.OperationData;

/*
 * union CryptoOperation - Type that describes the different operations that can be performed along
 *                         with its required parameters. It will be used to construct a vector of
 *                         operation that are executed sequentially.
 *
 * @SetMemoryBuffer:
 *      Sets a memory buffer to operate on. References to positins of this memory buffer can be used
 *      when setting the parameters for <code>UpdateAad</code>, <code>UpdateData</code>,
 *      <code>Finish</code> and <code>CopyData</code>
 * @SetOperationParameters:
 *      Sets the parameters for the current operation, for more info on specific parameters see
 *      <code>OperationParameters</code>.
 * @SetAad:
 *      Adds additional authenticated data. This call is only valid after a
 *      <code>SetOperationParameters</code> of type <code>SymmetricAuthOperationParameters</code>.
 * @SetDataOutput:
 *      Adds ousput buffers to store resutls form the operation. This call is only valid after a
 *      <code>SetOperationParameters</code> and it needs to be done before calling
 *      <code>SetDataInput</code>
 * @SetDataInput:
 *      Adds data to the operation for procesing. This call is only valid after a
 *      <code>SetOperationParameters</code> and it will trigger the operation, so output buffers
 *      need to be set first.
 * @CopyData:
 *      Copies data from input to output.
 * @Finish:
 *      Finalizes a cryptographic operation in flight. Becuase operations are initiated with a call
 *      to <code>SetOperationParameters</code>, a <code>finish</code> element is only valid after a
 *      <code>SetOperationParameters</code> element.
 * @DestroyContext:
 *      Specifies that we do not want to continue using this context anymore. The result of this
 *      call is that all resources are freed after finishing operating on the set of commands and no
 *      context is returned to the caller.
 */
union CryptoOperation {
    MemoryBufferParameter SetMemoryBuffer;
    OperationParameters SetOperationParameters;
    OperationData SetAad;
    OperationData SetDataOutput;
    OperationData SetDataInput;
    FinishParameters Finish;
    CopyParameters CopyData;
    EmptyEnum DestroyContext;
}
