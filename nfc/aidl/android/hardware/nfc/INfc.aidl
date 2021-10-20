/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.nfc;

import android.hardware.nfc.INfcClientCallback;
import android.hardware.nfc.NfcCloseType;
import android.hardware.nfc.NfcConfig;
import android.hardware.nfc.NfcStatus;

/**
 * All binder calls in the HAL may return a service-specific error statuses from the
 * NfcStatus integers defined in NfcStatus. Each method states which status can be returned
 * and under which circumstances.
 **/

@VintfStability
interface INfc {
    /**
     * Opens the NFC controller device and performs initialization.
     * This may include patch download and other vendor-specific initialization.
     *
     * If open completes successfully, the controller should be ready to perform
     * NCI initialization - ie accept CORE_RESET and subsequent commands through
     * the write() call.
     *
     * If open() returns without error, the NCI stack will wait for a
     * NfcEvent.OPEN_CPLT before continuing.
     *
     */
    void open(in INfcClientCallback clientCallback);

    /**
     * Close the NFC controller. Should free all resources.
     */
    void close(in NfcCloseType type);

    /**
     * Enable Power off use cases and close the NFC controller.
     * Should free all resources.
     *
     * This call must enable NFC functionality for off host usecases in power
     * off use cases, if the device supports power off use cases. If the
     * device doesn't support power off use cases, this call should be same as
     * close()
     */

    /**
     * Grant HAL the exclusive control to send NCI commands.
     * Called in response to NfcEvent.REQUEST_CONTROL.
     * Must only be called when there are no NCI commands pending.
     * NfcEvent.RELEASE_CONTROL will notify when HAL no longer needs exclusive control.
     */
    void controlGranted();

    /**
     * coreInitialized() is called after the CORE_INIT_RSP is received from the
     * NFCC. At this time, the HAL can do any chip-specific configuration.
     *
     * If coreInitialized() returns without error, the NCI stack will wait for a
     * NfcEvent.POST_INIT_CPLT before continuing.
     *
     * If coreInitialized() reports a service-specific error NfcStatus::FAILED,
     * the NCI stack will continue immediately.
     *
     */
    void coreInitialized(in byte[] data);

    /**
     * Clears the NFC chip.
     *
     * Must be called during factory reset and/or before the first time the HAL is
     * initialized after a factory reset
     */
    void factoryReset();

    /**
     * Fetches vendor specific configurations.
     * @return NfcConfig indicates support for certain features and
     * populates the vendor specific configs.
     */
    NfcConfig getConfig();

    /**
     * Restart controller by power cyle;
     * NfcEvent.OPEN_CPLT will notify when operation is complete.
     */
    void powerCycle();

    /**
     * preDiscover is called every time before starting RF discovery.
     * It is a good place to do vendor-specific configuration that must be
     * performed every time RF discovery is about to be started.
     *
     * If preDiscover() returns withour error, the NCI stack will wait for a
     * NfcEvent.PREDISCOVER_CPLT before continuing.
     *
     * If preDiscover() reports a service-specific error NfcStatus::FAILED,
     * the NCI stack will start RF discovery immediately.
     */
    void preDiscover();

    /**
     * Performs an NCI write.
     *
     * This method may queue writes and return immediately. The only
     * requirement is that the writes are executed in order.
     *
     * @return number of bytes written to the NFCC
     */
    int write(in byte[] data);
}
