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

import android.hardware.security.see.hwcrypto.CryptoOperation;

/*
 * interface ICryptoOperationContext - Interface that can be used to execute more commands. It
 *                                     stores the state from the previously executed commands, which
 *                                     means that if <code>CryptoOperation::Finish</code> has not
 *                                     been issued, it will use the parameters from the last issued
 *                                     <code>CryptoOperation::SetOperationParameters</code>.
 *                                     Additionaly, this interface also remember a memory buffer
 *                                     previously mapped with
 *                                     <code>CryptoOperation::SetMemoryBuffer</code>.
 *
 */
interface ICryptoOperationContext {}
