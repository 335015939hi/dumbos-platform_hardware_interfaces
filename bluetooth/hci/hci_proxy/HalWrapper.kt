/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

package android.hardware.bluetooth.hci.hci_proxy

import java.io.DataOutputStream

abstract class HalWrapper {
    
    abstract fun initialize(outputStream: DataOutputStream)
    abstract fun close()
    abstract fun sendHciCommand(data: ByteArray)
    abstract fun sendAclData(data: ByteArray)
    abstract fun sendScoData(data: ByteArray)
    abstract fun sendIsoData(data: ByteArray)
}