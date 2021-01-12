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

import android.hardware.health.BatteryHealth;
import android.hardware.health.BatteryStatus;
import android.hardware.health.BatteryCapacityLevel;
import android.hardware.health.DiskStats;
import android.hardware.health.StorageInfo;

/**
 * Health Information.
 */
@VintfStability
parcelable HealthInfo {
    /**
     * AC charger state - 'true' if online
     */
    boolean chargerAcOnline;
    /**
     * USB charger state - 'true' if online
     */
    boolean chargerUsbOnline;
    /**
     * Wireless charger state - 'true' if online
     */
    boolean chargerWirelessOnline;
    /**
     * Maximum charging current supported by charger in uA
     */
    int maxChargingCurrent;
    /**
     * Maximum charging voltage supported by charger in uV
     */
    int maxChargingVoltage;
    android.hardware.health.BatteryStatus batteryStatus;
    android.hardware.health.BatteryHealth batteryHealth;
    /**
     * 'true' if battery is present
     */
    boolean batteryPresent;
    /**
     * Remaining battery capacity in percent
     */
    int batteryLevel;
    /**
     * Instantaneous battery voltage in millivolts (mV).
     *
     * Historically, the unit of this field is microvolts (uV), but all
     * clients and implementations uses millivolts in practice, making it
     * the de-facto standard.
     */
    int batteryVoltage;
    /**
     * Instantaneous battery temperature in tenths of degree celcius
     */
    int batteryTemperature;
    /**
     * Instantaneous battery current in uA
     */
    int batteryCurrent;
    /**
     * Battery charge cycle count
     */
    int batteryCycleCount;
    /**
     * Battery charge value when it is considered to be "full" in uA-h
     */
    int batteryFullCharge;
    /**
     * Instantaneous battery capacity in uA-h
     */
    int batteryChargeCounter;
    /**
     * Battery technology, e.g. "Li-ion, Li-Poly" etc.
     */
    String batteryTechnology;
    /**
     * Average battery current in uA. Will be 0 if unsupported.
     */
    int batteryCurrentAverage;
    /**
     * Disk Statistics. Will be an empty vector if unsupported.
     */
    DiskStats[] diskStats;
    /**
     * Information on storage devices. Will be an empty vector if
     * unsupported.
     */
    StorageInfo[] storageInfos;
    /**
     * Battery capacity level. See BatteryCapacityLevel for more details.
     */
    BatteryCapacityLevel batteryCapacityLevel;

    /**
     * Value of {@link #batteryChargeTimeToFullNowSeconds} if it is not
     * supported.
     */
    const int BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED = -1;
    /**
     * Estimated time to fully charge the device (in seconds).
     * Value must be BATTERY_CHARGE_TIME_TO_FULL_NOW_SECONDS_UNSUPPORTED if and
     * only if the implementation is unsupported.
     * Value must be 0 if and only if batteryCapacityLevel is FULL or UNKNOWN.
     * Otherwise, value must be positive.
     */
    long batteryChargeTimeToFullNowSeconds;
    /**
     * Estimated battery full charge design capacity (in microamp hours, uAh).
     * Value must be 0 if unknown.
     * Value must be greater than 100 000 uAh if known.
     * Value must be less than 100 000 000 uAh if known.
     */
    int batteryFullChargeDesignCapacityUah;
}

