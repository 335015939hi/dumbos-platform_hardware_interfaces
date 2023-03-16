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

import android.bluetooth.BluetoothAdapter
import android.os.Bundle
import android.os.Debug
import android.os.ServiceManager
import android.util.Log
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.runner.MonitoringInstrumentation

import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.DataInputStream
import java.io.DataOutputStream
import java.io.InputStream
import java.io.OutputStream
import java.net.ServerSocket
import java.net.Socket

import kotlin.concurrent.thread

@kotlinx.coroutines.ExperimentalCoroutinesApi
class Main : MonitoringInstrumentation() {

  private val TAG = "HciProxy"

  override fun onCreate(arguments: Bundle) {
    super.onCreate(arguments)

    // Activate debugger.
    if (arguments.getString("debug").toBoolean()) {
      Log.i(TAG, "Waiting for debugger to connect...")
      Debug.waitForDebugger()
      Log.i(TAG, "Debugger connected")
    }

    // Start instrumentation thread.
    start()
  }

  override fun onStart() {
    super.onStart()

    val uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation()
    // Adopt all the permissions of the shell
    uiAutomation.adoptShellPermissionIdentity()

    BluetoothAdapter.getDefaultAdapter().disable(true)

    val halWrapper = ServiceManager.waitForDeclaredService("android.hardware.bluetooth.IBluetoothHci/default")?.let{
      val service = android.hardware.bluetooth.IBluetoothHci.Stub.asInterface(it)
      AidlWrapper(service)
    } ?: android.hardware.bluetooth.V1_1.IBluetoothHci.getService(true)?.let {
      HidlWrapper_1_1(it)
    }?: android.hardware.bluetooth.V1_0.IBluetoothHci.getService(true)?.let {
      HidlWrapper_1_0(it)
    }

    if (halWrapper == null) {
      Log.e(TAG, "Unable to get HAL instance")
      return
    }

    thread { listener(halWrapper) }.join()
  }

  fun listener(halWrapper: HalWrapper) {
      val serverSocket = ServerSocket(9100)
      Log.d(TAG, "Server created")

      while(true) {
        try {
          val client = serverSocket.accept()
          Log.d(TAG, "New client")
          val inputStream = DataInputStream(BufferedInputStream(client.getInputStream()))
          val outputStream = DataOutputStream(BufferedOutputStream(client.getOutputStream()))

          halWrapper.initialize(outputStream)
          
          // read is blocking, because multiple clients is unexpected
          read(inputStream, halWrapper)

          client.close()
        } catch (e: Exception) {
          Log.e(TAG, e.toString())
        } finally {
          Log.d(TAG, "Socket closed")
          halWrapper.close()
        }
      }
  }

  fun read(inputStream: DataInputStream, halWrapper: HalWrapper) {
    while (true) {
      try {
        val type = inputStream.readByte()
        val buf = ByteArray(inputStream.available())
        inputStream.read(buf)
        when (type) {
          HciConstants.HCI_PACKET_TYPE_COMMAND -> halWrapper.sendHciCommand(buf)
          HciConstants.HCI_PACKET_TYPE_ACL_DATA -> halWrapper.sendAclData(buf)
          HciConstants.HCI_PACKET_TYPE_SCO_DATA -> halWrapper.sendScoData(buf)
          HciConstants.HCI_PACKET_TYPE_ISO_DATA -> halWrapper.sendIsoData(buf)
          else -> Log.e(TAG, "Unsupported packet type $type")
        }
      } catch (e: Exception) {
        Log.e(TAG, e.toString())
        break
      }
    }

  }
}