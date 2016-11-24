#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import time
from Queue import Queue

from threading import Thread, Condition

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

PASSTHROUGH_MODE_KEY = "passthrough_mode"

class BluetoothHidlBasicTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the Bluetooth HIDL HAL."""

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer Bluetooth service."""
        self.dut = self.registerController(android_device)[0]

        self.getUserParams(opt_param_names=[PASSTHROUGH_MODE_KEY])

        self.dut.shell.InvokeTerminal("one")
        #self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode
        #self.dut.shell.one.Execute("svc bluetooth disable")  # Turn off
        #time.sleep(2)

        if getattr(self, PASSTHROUGH_MODE_KEY, True):
            self.dut.shell.one.Execute(
                "setprop vts.hal.vts.hidl.get_stub true")
        else:
            self.dut.shell.one.Execute(
                "setprop vts.hal.vts.hidl.get_stub false")

        self.dut.hal.InitHidlHal(target_type="bluetooth",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 target_package="android.hardware.bluetooth",
                                 target_component_name="IBluetoothHci",
                                 bits=64)

    def tearDownClass(self):
        """Turns off the framework-layer Bluetooth service."""
        #self.dut.shell.one.Execute("svc bluetooth disable")  # Turn off

    def testBase(self):
        """Send a reset and wait for an event."""

        event_queue = Queue()

        def hciEventReceived(HciEvent):
            global event_queue
            logging.info("callback hciEventReceived")
            logging.info("arg0 %s", HciEvent)
            event_queue.put(HciEvent)

        def aclDataReceived(AclData):
            logging.info("callback aclDataReceived")
            logging.info("arg0 %s", AclData)

        def scoDataReceived(ScoData):
            logging.info("callback scoDataReceived")
            logging.info("arg0 %s", ScoData)

        def wait_for_event():
            global event_queue
            logging.info("waiting for event callback")

            event = event_queue.get()
            event_queue.task_done()
            logging.info("event %s", event)

        client_callbacks = self.dut.hal.bluetooth.GetHidlCallbackInterface(
            "IBluetoothHciCallbacks",
            hciEventReceived=hciEventReceived,
            scoDataReceived=scoDataReceived,
            aclDataRecevied=aclDataRecevied)

        result = self.dut.hal.bluetooth.initialize(client_callbacks)
        logging.info("initialize result: %s", result)

        self.dut.hal.bluetooth.sendHciCommand([0x03, 0x0C, 0x00])
        logging.info("sent reset")

        wait_for_event(True, 30) # Time out after 30 seconds

        self.dut.hal.bluetooth.close()
        logging.info("called close")

if __name__ == "__main__":
    test_runner.main()
