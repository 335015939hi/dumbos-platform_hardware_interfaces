/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.hypervisor;

/**
 * HAL to support Android running in virtualization.
 *
 * This HAL is only meant to be implemented in cases where
 * Android is running in a virtualized setting, either providing
 * host services to other VMs, or running as a guest virtual machine
 * under a hypervisor.
 *
 * In those scenarios, this HAL can be used to provide information
 * about the underlying virtualization environment for use by system
 * components.
 */
@VintfStability
interface IHypervisor {
    /**
     * Get the version string of the hypervisor implementation.
     *
     * This string can be a free-form descriptor of the host/hypervisor system that is
     * being used to run the current Android instance. It may be the Android version
     * if Android itself is running as the host, or the version information for a different
     * operating system or hypervisor if Android is running purely as a guest VM.
     *
     * Clients SHOULD NOT make any assumption about the format of this string nor attempt to parse
     * it in order to provide workarounds for environment issues. It SHOULD be treated as
     * best-effort and informational only, e.g. suitable for inclusion in logs, or being shown to
     * users as part of system information UIs.
     *
     * @return The version string of the device implementation. Must have nonzero length.
     * @throws ServiceSpecificException with one of the following ErrorStatus values:
     *     - DEVICE_UNAVAILABLE if the version cannot be retrieved
     */
    String getVersionString();
}
