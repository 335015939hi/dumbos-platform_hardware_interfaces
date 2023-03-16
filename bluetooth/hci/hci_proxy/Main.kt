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
import android.content.Context
import android.hardware.bluetooth.IBluetoothHci
import android.hardware.bluetooth.IBluetoothHciCallbacks
import android.os.Bundle
import android.os.Debug
import android.os.ServiceManager
import android.util.Log
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.runner.MonitoringInstrumentation

import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.DataInputStream
import java.io.DataOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.net.ServerSocket
import java.net.Socket
import java.util.ArrayList
import java.util.Arrays
import java.util.stream.Collectors

import kotlin.concurrent.thread

@kotlinx.coroutines.ExperimentalCoroutinesApi
class Main : MonitoringInstrumentation() {

  companion object {
    private val TAG = "HciProxy"

    private val HCI_PACKET_TYPE_COMMAND : Byte = 1
    private val HCI_PACKET_TYPE_ACL_DATA : Byte = 2
    private val HCI_PACKET_TYPE_SCO_DATA : Byte = 3
    private val HCI_PACKET_TYPE_EVENT : Byte = 4
    private val HCI_PACKET_TYPE_ISO_DATA : Byte = 5
  }

  private var service: IBluetoothHci? = null


  class BluetoothHciCallbacksAidl(val rawOut:OutputStream): IBluetoothHciCallbacks.Stub() {
    private val out: DataOutputStream

    init {
      out = DataOutputStream(BufferedOutputStream(rawOut))
    }

    override fun initializationComplete(status:Int) {
        Log.i(TAG, "AIDL client init complete, status=$status")
    }

    override fun getInterfaceHash() = IBluetoothHciCallbacks.HASH
    override fun getInterfaceVersion() = IBluetoothHciCallbacks.VERSION

    override fun hciEventReceived(data: ByteArray) {
      try {
        out.writeByte(HCI_PACKET_TYPE_EVENT.toInt())
        out.write(data)
        out.flush()
      } catch (e: IOException) {
        Log.e(TAG, e.toString())
      }
    }

    override fun aclDataReceived(data: ByteArray) {
      try {
        out.writeByte(HCI_PACKET_TYPE_ACL_DATA.toInt())
        out.write(data)
        out.flush()
      } catch (e: IOException) {
        Log.e(TAG, e.toString())
      }
    }

    override fun scoDataReceived(data: ByteArray) {
      try {
        out.writeByte(HCI_PACKET_TYPE_SCO_DATA.toInt())
        out.write(data)
        out.flush()
      } catch (e: IOException) {
        Log.e(TAG, e.toString())
      }
    }

    override fun isoDataReceived(data: ByteArray) {
      try {
        out.writeByte(HCI_PACKET_TYPE_ISO_DATA.toInt())
        out.write(data)
        out.flush()
      } catch (e: IOException) {
        Log.e(TAG, e.toString())
      }
    }
  }

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

    val context: Context = getApplicationContext()
    val uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation()
    // Adopt all the permissions of the shell
    uiAutomation.adoptShellPermissionIdentity()

    BluetoothAdapter.getDefaultAdapter().disable(true)

    service = try {
      IBluetoothHci.Stub.asInterface(
          ServiceManager.waitForDeclaredService(
              "android.hardware.bluetooth.IBluetoothHci/default"))
    } catch (e:Exception) {
      Log.e(TAG, e.toString())
      return
    }

    thread { listener() }.join()
  }

  fun listener() {
      val serverSocket = ServerSocket(9100)
      Log.d(TAG, "Server created")

      while(true) {
        try {
          val client = serverSocket.accept()
          Log.d(TAG, "New client")
          val intputStream : InputStream = client.getInputStream()
          val outputStream : OutputStream = client.getOutputStream()
          val callbacks = BluetoothHciCallbacksAidl(outputStream)

          service?.initialize(callbacks)
          
          // reader is blocking, because multiple clients is unexpected
          reader(intputStream)

          client.close()
        } catch (e: Exception) {
          Log.e(TAG, e.toString())
        } finally {
          Log.d(TAG, "Socket closed")
          service?.close()
        }
      }
  }

  fun reader(intputStream: InputStream) {
    val stream = DataInputStream(BufferedInputStream(intputStream))
    while (true) {
      try {
        val type = stream.readByte()
        val buf = ByteArray(stream.available())
        stream.read(buf)
        when (type) {
          HCI_PACKET_TYPE_COMMAND -> service?.sendHciCommand(buf)
          HCI_PACKET_TYPE_ACL_DATA -> service?.sendAclData(buf)
          HCI_PACKET_TYPE_SCO_DATA -> service?.sendScoData(buf)
          HCI_PACKET_TYPE_ISO_DATA -> service?.sendIsoData(buf)
          else -> Log.e(TAG, "Unsupported packet type $type")
        }
      } catch (e: Exception) {
        Log.e(TAG, e.toString())
        break
      }
    }

  }
}