/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.thermal;

import android.hardware.thermal.CoolingDevice;
import android.hardware.thermal.CoolingType;
import android.hardware.thermal.IThermalChangedCallback;
import android.hardware.thermal.Temperature;
import android.hardware.thermal.TemperatureThreshold;
import android.hardware.thermal.TemperatureType;

@VintfStability
interface IThermal {
    /**
     * Retrieves the cooling devices information.
     *
     * @return devices If succeed, it's filled with the
     *    current cooling device information. The order of built-in cooling
     *    devices in the list must be kept the same regardless the number
     *    of calls to this method even if they go offline, if these devices
     *    exist on boot. The method always returns and never removes from
     *    the list such cooling devices.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, getMessage() must be populated with the human-readable
     *    error message.
     */
    CoolingDevice[] getCoolingDevices();

    /**
     * Retrieves the cooling devices information.
     *
     * @param filterType whether to filter the result for a given type.
     * @param type the CoolingDevice such as CPU/GPU.
     *
     * @return devices If succeed, it's filled with the current
     *    cooling device information. The order of built-in cooling
     *    devices in the list must be kept the same regardless of the number
     *    of calls to this method even if they go offline, if these devices
     *    exist on boot. The method always returns and never removes from
     *    the list such cooling devices.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    CoolingDevice[] getCurrentCoolingDevices(in boolean filterType, in CoolingType type);

    /**
     * Retrieves temperatures in Celsius.
     *
     * @param filterType whether to filter the result for a given type.
     * @param type the TemperatureType such as battery or skin.
     *
     * @return temperatures If succeed, it's filled with the
     *    current temperatures. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless of the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    Temperature[] getCurrentTemperatures(in boolean filterType, in TemperatureType type);

    /**
     * Retrieves static temperature thresholds in Celsius.
     *
     * @param filterType whether to filter the result for a given type.
     * @param type the TemperatureType such as battery or skin.
     *
     * @return temperatureThresholds If succeed, it's filled with the
     *    temperatures thresholds. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless of the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures. The thresholds
     *    are returned as static values and must not change across calls. The actual
     *    throttling state is determined in device thermal mitigation policy/agorithm
     *    which might not be simple thresholds so these values Thermal HAL provided
     *    may not be accurate to detemin the throttling status. To get accurate
     *    throttling status, use getCurrentTemperatures or registerThermalChangedCallback
     *    and listen to the callback.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    TemperatureThreshold[] getTemperatureThresholds(in boolean filterType, in TemperatureType type);

    /**
     * Retrieves temperatures in Celsius.
     *
     * @return temperatures If succeed, it's filled with the
     *    current temperatures. The order of temperatures of built-in
     *    devices (such as CPUs, GPUs and etc.) in the list must be kept
     *    the same regardless the number of calls to this method even if
     *    they go offline, if these devices exist on boot. The method
     *    always returns and never removes such temperatures.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    Temperature[] getTemperatures();

    /**
     * Register an IThermalChangedCallback, used by the Thermal HAL
     * to receive thermal events when thermal mitigation status changed.
     * Multiple registrations with different IThermalChangedCallback must be allowed.
     * Multiple registrations with same IThermalChangedCallback is not allowed, client
     * should unregister the given IThermalChangedCallback first.
     *
     * @param callback the IThermalChangedCallback to use for receiving
     *    thermal events (nullptr callback will lead to failure with status code FAILURE).
     * @param filterType if filter for given sensor type.
     * @param type the type to be filtered.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    void registerThermalChangedCallback(
            in IThermalChangedCallback callback, in boolean filterType, in TemperatureType type);

    /**
     * Unregister an IThermalChangedCallback, used by the Thermal HAL
     * to receive thermal events when thermal mitigation status changed.
     *
     * @param callback the IThermalChangedCallback used for receiving
     *    thermal events (nullptr callback will lead to failure with status code FAILURE).
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, the getMessage() must be populated with the human-readable
     *    error message.
     */
    void unregisterThermalChangedCallback(in IThermalChangedCallback callback);
}
