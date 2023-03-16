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

import android.hardware.bluetooth.IBluetoothHci
import android.hardware.bluetooth.IBluetoothHciCallbacks
import android.util.Log
import java.io.DataOutputStream

class AidlWrapper(service: IBluetoothHci) : HalWrapper() {
  companion object {
    private val TAG = "HciProxyAidl"
  }

  private val service: IBluetoothHci
  private var callbacks: BluetoothHciCallbacks? = null

  init {
    this.service = service
  }

  class BluetoothHciCallbacks(out: DataOutputStream) : IBluetoothHciCallbacks.Stub() {
    private val out: DataOutputStream

    init {
      this.out = out
    }

    override fun initializationComplete(status: Int) {
      Log.i(TAG, "HAL client init complete, status=$status")
    }

    override fun getInterfaceHash() = IBluetoothHciCallbacks.HASH
    override fun getInterfaceVersion() = IBluetoothHciCallbacks.VERSION

    fun writePacketWithType(typeByte: Byte, data: ByteArray) {
      out.writeByte(typeByte.toInt())
      out.write(data)
      out.flush()
    }

    override fun hciEventReceived(data: ByteArray) =
      writePacketWithType(HciConstants.HCI_PACKET_TYPE_EVENT, data)

    override fun aclDataReceived(data: ByteArray) =
      writePacketWithType(HciConstants.HCI_PACKET_TYPE_ACL_DATA, data)

    override fun scoDataReceived(data: ByteArray) =
      writePacketWithType(HciConstants.HCI_PACKET_TYPE_SCO_DATA, data)

    override fun isoDataReceived(data: ByteArray) =
      writePacketWithType(HciConstants.HCI_PACKET_TYPE_ISO_DATA, data)
  }

  override fun initialize(outputStream: DataOutputStream) {
    this.callbacks = BluetoothHciCallbacks(outputStream)
    service.initialize(this.callbacks)
  }

  override fun close() {
    service.close()
    this.callbacks = null
  }

  override fun sendHciCommand(data: ByteArray) = service.sendHciCommand(data)
  override fun sendAclData(data: ByteArray) = service.sendAclData(data)
  override fun sendScoData(data: ByteArray) = service.sendScoData(data)
  override fun sendIsoData(data: ByteArray) = service.sendIsoData(data)
}
