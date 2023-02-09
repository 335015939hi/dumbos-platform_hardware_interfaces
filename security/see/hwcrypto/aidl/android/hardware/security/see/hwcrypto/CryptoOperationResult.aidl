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

import android.hardware.security.see.hwcrypto.ICryptoOperationContext;
import android.hardware.security.see.hwcrypto.types.HalErrorCode;
import android.hardware.security.see.hwcrypto.types.NullableLong;

/*
 * parcelable CryptoOperationResult - Type that describes the result of a set of crypto operations.
 *
 * @error_code:
 *      Result of the operation. If an error ocurred, it contains the error code of the failing
 *      step.
 * @failing_step:
 *      Index indicating the first failing step on a set of commands. No more commands would have
 *      been executed after thsi one failed. Only available is error_code is not OK.
 * @context:
 *      Object that can be used to issue more operations on a future call.
 */
parcelable CryptoOperationResult {
    HalErrorCode error_code;
    @nullable NullableLong failing_step;
    @nullable ICryptoOperationContext context;
}
