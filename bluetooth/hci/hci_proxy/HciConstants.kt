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

class HciConstants {
  companion object {
    public val HCI_PACKET_TYPE_COMMAND: Byte = 1
    public val HCI_PACKET_TYPE_ACL_DATA: Byte = 2
    public val HCI_PACKET_TYPE_SCO_DATA: Byte = 3
    public val HCI_PACKET_TYPE_EVENT: Byte = 4
    public val HCI_PACKET_TYPE_ISO_DATA: Byte = 5
  }
}