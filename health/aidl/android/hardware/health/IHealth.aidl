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

package android.hardware.health;

import android.hardware.health.BatteryStatus;
import android.hardware.health.DiskStats;
import android.hardware.health.HealthConfig;
import android.hardware.health.HealthInfo;
import android.hardware.health.IHealthInfoCallback;
import android.hardware.health.StorageInfo;

/**
 * IHealth manages health info and posts events on registered callbacks.
 *
 * Passthrough implementations are not required to send health info to all
 * callbacks periodically, but they must do so when update() is called.
 * Binderized implementations must send health info to all callbacks
 * periodically. The intervals between two notifications must be retrieved from
 * the passthrough implementation through the getHealthConfig() function.
 */
@VintfStability
interface IHealth {
    /** Status code for function. The operation is not supported. */
    const int STATUS_NOT_SUPPORTED = 1;

    /** Status code for function. The operation encounters an unknown error. */
    const int STATUS_UNKNOWN = 2;

    /**
     * Status code for function.
     * The provided callback object is not previously registered.
     */
    const int STATUS_NOT_FOUND = 3;

    /**
     * Status code for function.
     * A registered callback object is dead.
     */
    const int STATUS_CALLBACK_DIED = 4;

    /**
     * Register a callback for any health info events.
     *
     * Registering a new callback must not unregister the old one; the old
     * callback remains registered until one of the following happens:
     * - A client explicitly calls {@link #unregisterCallback} to unregister it.
     * - The client process that hosts the callback dies.
     *
     * @param callback the callback to register.
     * @return If error, status code is
     *         STATUS_UNKNOWN for other errors.
     */
    void registerCallback(in IHealthInfoCallback callback);

    /**
     * Explicitly unregister a callback that is previously registered through
     * {@link #registerCallback}.
     *
     * @param callback the callback to unregister
     * @return If error, status code is
     *         STATUS_NOT_FOUND if callback is not registered previously,
     *         STATUS_UNKNOWN for other errors.
     */
    void unregisterCallback(in IHealthInfoCallback callback);

    /**
     * Schedule update.
     *
     * When update() is called, the service must notify all registered callbacks
     * with the most recent health info.
     *
     * @return If error, status code is
     *         STATUS_CALLBACK_DIED if any registered callback is dead,
     *         STATUS_UNKNOWN for other errors.
     */
    void update();

    /**
     * Get remaining battery capacity percentage of total capacity
     * (with no fractional part).
     *
     * @return remaining battery capacity if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         STATUS_UNKNOWN for other errors.
     */
    int getCapacity();

    /**
     * Get battery capacity in microampere-hours(µAh).
     *
     * @return battery capacity if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         STATUS_UNKNOWN for other errors.
     */
    int getChargeCounter();

    /**
     * Get battery charge status.
     *
     * @return charge status if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         STATUS_UNKNOWN other errors.
     */
    android.hardware.health.BatteryStatus getChargeStatus();

    /**
     * Get average battery current in microamperes(µA).
     *
     * Positive values indicate net current entering the battery from a charge
     * source, negative values indicate net current discharging from the
     * battery. The time period over which the average is computed may depend on
     * the fuel gauge hardware and its configuration.
     *
     * @return average battery current if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         STATUS_UNKNOWN for other errors.
     */
    int getCurrentAverage();

    /**
     * Get instantaneous battery current in microamperes(µA).
     *
     * Positive values indicate net current entering the battery from a charge
     * source, negative values indicate net current discharging from the
     * battery.
     *
     * @return instantaneous battery current if successful,
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         STATUS_UNKNOWN for other errors.
     */
    int getCurrentNow();

    /**
     * Gets disk statistics (number of reads/writes processed, number of I/O
     * operations in flight etc).
     *
     * @return vector of disk statistics if successful.
     *         The mapping is index 0->sda, 1->sdb and so on.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported,
     *         STATUS_UNKNOWN other errors.
     */
    DiskStats[] getDiskStats();

    /**
     * Get battery remaining energy in nanowatt-hours.
     *
     * @return remaining energy if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported,
     *         STATUS_UNKNOWN for other errors.
     */
    long getEnergyCounter();

    /**
     * Get configuration of this HAL.
     *
     * @return HAL configuration if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this API is not supported,
     *         STATUS_UNKNOWN for other errors.
     */
    HealthConfig getHealthConfig();

    /**
     * Get Health Information.
     *
     * @return Health information if successful
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this API is not supported,
     *         STATUS_UNKNOWN for other errors.
     */
    HealthInfo getHealthInfo();

    /**
     * Get storage info.
     *
     * @return vector of StorageInfo structs if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this property is not supported,
     *         STATUS_UNKNOWN other errors.
     *         If error, status code is:
     */
    StorageInfo[] getStorageInfo();

    /**
     * Return whether the screen should be kept on in charger mode.
     *
     * @return whether screen should be kept on if successful.
     *         If error, status code is:
     *         STATUS_NOT_SUPPORTED if this API is not supported,
     *         STATUS_UNKNOWN for other errors.
     */
    boolean shouldKeepScreenOn();
}
