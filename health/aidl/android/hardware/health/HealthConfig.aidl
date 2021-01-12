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

/**
 * Combined configuration of a health HAL implementation.
 */
@VintfStability
parcelable HealthConfig {
    /**
     * periodicChoresIntervalFast is used while the device is not in
     * suspend, or in suspend and connected to a charger (to watch for battery
     * overheat due to charging)
     */
    int periodicChoresIntervalFast;
    /**
     * periodicChoresIntervalSlow is used when the device is in suspend and
     * not connected to a charger (to watch for a battery drained to zero
     * remaining capacity).
     */
    int periodicChoresIntervalSlow;
    /**
     * power_supply sysfs attribute file paths. Set these to specific paths
     * to use for the associated battery parameters. Clients must search
     * for appropriate power_supply attribute files to use, for any paths
     * left empty after the HAL is initialized.
     *
     *
     * batteryStatusPath - file path to read battery charging status.
     * (POWER_SUPPLY_PROP_STATUS)
     */
    String batteryStatusPath;
    /**
     * batteryHealthPath - file path to read battery health.
     * (POWER_SUPPLY_PROP_HEALTH)
     */
    String batteryHealthPath;
    /**
     * batteryPresentPath - file path to read battery present status.
     * (POWER_SUPPLY_PROP_PRESENT)
     */
    String batteryPresentPath;
    /**
     * batteryCapacityPath - file path to read remaining battery capacity.
     * (POWER_SUPPLY_PROP_CAPACITY)
     */
    String batteryCapacityPath;
    /**
     * batteryVoltagePath - file path to read battery voltage.
     * (POWER_SUPPLY_PROP_VOLTAGE_NOW)
     */
    String batteryVoltagePath;
    /**
     * batteryTemperaturePath - file path to read battery temperature in tenths
     * of degree celcius. (POWER_SUPPLY_PROP_TEMP)
     */
    String batteryTemperaturePath;
    /**
     * batteryTechnologyPath - file path to read battery technology.
     * (POWER_SUPPLY_PROP_TECHNOLOGY)
     */
    String batteryTechnologyPath;
    /**
     * batteryCurrentNowPath - file path to read battery instantaneous current.
     * (POWER_SUPPLY_PROP_CURRENT_NOW)
     */
    String batteryCurrentNowPath;
    /**
     * batteryCurrentAvgPath - file path to read battery average current.
     * (POWER_SUPPLY_PROP_CURRENT_AVG)
     */
    String batteryCurrentAvgPath;
    /**
     * batteryChargeCounterPath - file path to read battery accumulated charge.
     * (POWER_SUPPLY_PROP_CHARGE_COUNTER)
     */
    String batteryChargeCounterPath;
    /**
     * batteryFullChargerPath - file path to read battery charge value when it
     * is considered to be full. (POWER_SUPPLY_PROP_CHARGE_FULL)
     */
    String batteryFullChargePath;
    /**
     * batteryCycleCountPath - file path to read battery charge cycle count.
     * (POWER_SUPPLY_PROP_CYCLE_COUNT)
     */
    String batteryCycleCountPath;
    /**
     * Minimum battery level for charger to reboot into Android (in percent).
     * Value should be in range [0, 100].
     */
    int bootMinCap;
}

