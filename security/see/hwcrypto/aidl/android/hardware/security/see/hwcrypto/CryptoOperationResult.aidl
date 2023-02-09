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

/*
 * parcelable CryptoOperationResult - Type that describes the result of a set of crypto operations.
 *
 * @context:
 *      Object that can be used to issue more operations on a future call.
 */
parcelable CryptoOperationResult {
    @nullable ICryptoOperationContext context;
}
