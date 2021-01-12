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

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.health;

public class Translate {
    static public android.hardware.health.StorageAttribute h2aTranslate(
            android.hardware.health.V2_0.StorageAttribute in) {
        android.hardware.health.StorageAttribute out =
                new android.hardware.health.StorageAttribute();
        out.isInternal = in.isInternal;
        out.isBootDevice = in.isBootDevice;
        out.name = in.name;
        return out;
    }

    static public android.hardware.health.StorageInfo h2aTranslate(
            android.hardware.health.V2_0.StorageInfo in) {
        android.hardware.health.StorageInfo out = new android.hardware.health.StorageInfo();
        out.attr = h2aTranslate(in.attr);
        out.eol = in.eol;
        out.lifetimeA = in.lifetimeA;
        out.lifetimeB = in.lifetimeB;
        out.version = in.version;
        return out;
    }

    static public android.hardware.health.DiskStats h2aTranslate(
            android.hardware.health.V2_0.DiskStats in) {
        android.hardware.health.DiskStats out = new android.hardware.health.DiskStats();
        out.reads = in.reads;
        out.readMerges = in.readMerges;
        out.readSectors = in.readSectors;
        out.readTicks = in.readTicks;
        out.writes = in.writes;
        out.writeMerges = in.writeMerges;
        out.writeSectors = in.writeSectors;
        out.writeTicks = in.writeTicks;
        out.ioInFlight = in.ioInFlight;
        out.ioTicks = in.ioTicks;
        out.ioInQueue = in.ioInQueue;
        out.attr = h2aTranslate(in.attr);
        return out;
    }

    static public android.hardware.health.HealthInfo h2aTranslate(
            android.hardware.health.V2_1.HealthInfo in) {
        android.hardware.health.HealthInfo out = new android.hardware.health.HealthInfo();
        out.chargerAcOnline = in.legacy.legacy.chargerAcOnline;
        out.chargerUsbOnline = in.legacy.legacy.chargerUsbOnline;
        out.chargerWirelessOnline = in.legacy.legacy.chargerWirelessOnline;
        out.maxChargingCurrent = in.legacy.legacy.maxChargingCurrent;
        out.maxChargingVoltage = in.legacy.legacy.maxChargingVoltage;
        out.batteryStatus = in.legacy.legacy.batteryStatus;
        out.batteryHealth = in.legacy.legacy.batteryHealth;
        out.batteryPresent = in.legacy.legacy.batteryPresent;
        out.batteryLevel = in.legacy.legacy.batteryLevel;
        out.batteryVoltage = in.legacy.legacy.batteryVoltage;
        out.batteryTemperature = in.legacy.legacy.batteryTemperature;
        out.batteryCurrent = in.legacy.legacy.batteryCurrent;
        out.batteryCycleCount = in.legacy.legacy.batteryCycleCount;
        out.batteryFullCharge = in.legacy.legacy.batteryFullCharge;
        out.batteryChargeCounter = in.legacy.legacy.batteryChargeCounter;
        out.batteryTechnology = in.legacy.legacy.batteryTechnology;
        out.batteryCurrentAverage = in.legacy.batteryCurrentAverage;
        out.diskStats = new android.hardware.health.DiskStats[in.diskStats.length];
        for (int i = 0; i < in.diskStats.length; i++) {
            out.diskStats[i] = h2aTranslate(in.diskStats[i]);
        }
        out.diskStats = new android.hardware.health.StorageInfos[in.diskStats.length];
        for (int i = 0; i < in.storageInfos.length; i++) {
            out.storageInfos[i] = h2aTranslate(in.storageInfos[i]);
        }
        out.batteryCapacityLevel = in.batteryCapacityLevel;
        out.batteryChargeTimeToFullNowSeconds = in.batteryChargeTimeToFullNowSeconds;
        out.batteryFullChargeDesignCapacityUah = in.batteryFullChargeDesignCapacityUah;
        return out;
    }

    static public android.hardware.health.HealthConfig h2aTranslate(
            android.hardware.health.V2_1.HealthConfig in) {
        android.hardware.health.HealthConfig out = new android.hardware.health.HealthConfig();
        out.periodicChoresIntervalFast = in.battery.periodicChoresIntervalFast;
        out.periodicChoresIntervalSlow = in.battery.periodicChoresIntervalSlow;
        out.batteryStatusPath = in.battery.batteryStatusPath;
        out.batteryHealthPath = in.battery.batteryHealthPath;
        out.batteryPresentPath = in.battery.batteryPresentPath;
        out.batteryCapacityPath = in.battery.batteryCapacityPath;
        out.batteryVoltagePath = in.battery.batteryVoltagePath;
        out.batteryTemperaturePath = in.battery.batteryTemperaturePath;
        out.batteryTechnologyPath = in.battery.batteryTechnologyPath;
        out.batteryCurrentNowPath = in.battery.batteryCurrentNowPath;
        out.batteryCurrentAvgPath = in.battery.batteryCurrentAvgPath;
        out.batteryChargeCounterPath = in.battery.batteryChargeCounterPath;
        out.batteryFullChargePath = in.battery.batteryFullChargePath;
        out.batteryCycleCountPath = in.battery.batteryCycleCountPath;
        out.bootMinCap = in.bootMinCap;
        return out;
    }
}
