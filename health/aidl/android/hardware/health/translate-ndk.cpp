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

#include "android/hardware/health/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::health::BatteryStatus::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryStatus::CHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::CHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::DISCHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::DISCHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::NOT_CHARGING ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::NOT_CHARGING));
static_assert(aidl::android::hardware::health::BatteryStatus::FULL ==
              static_cast<aidl::android::hardware::health::BatteryStatus>(
                      ::android::hardware::health::V1_0::BatteryStatus::FULL));

static_assert(aidl::android::hardware::health::BatteryHealth::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryHealth::GOOD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::GOOD));
static_assert(aidl::android::hardware::health::BatteryHealth::OVERHEAT ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::OVERHEAT));
static_assert(aidl::android::hardware::health::BatteryHealth::DEAD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::DEAD));
static_assert(aidl::android::hardware::health::BatteryHealth::OVER_VOLTAGE ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::OVER_VOLTAGE));
static_assert(aidl::android::hardware::health::BatteryHealth::UNSPECIFIED_FAILURE ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::UNSPECIFIED_FAILURE));
static_assert(aidl::android::hardware::health::BatteryHealth::COLD ==
              static_cast<aidl::android::hardware::health::BatteryHealth>(
                      ::android::hardware::health::V1_0::BatteryHealth::COLD));

static_assert(aidl::android::hardware::health::BatteryCapacityLevel::UNSUPPORTED ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::UNSUPPORTED));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::UNKNOWN ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::UNKNOWN));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::CRITICAL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::CRITICAL));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::LOW ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::LOW));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::NORMAL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::NORMAL));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::HIGH ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::HIGH));
static_assert(aidl::android::hardware::health::BatteryCapacityLevel::FULL ==
              static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
                      ::android::hardware::health::V2_1::BatteryCapacityLevel::FULL));

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::StorageAttribute& in,
        aidl::android::hardware::health::StorageAttribute* out) {
    out->isInternal = static_cast<bool>(in.isInternal);
    out->isBootDevice = static_cast<bool>(in.isBootDevice);
    out->name = in.name;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::StorageInfo& in,
        aidl::android::hardware::health::StorageInfo* out) {
    if (!translate(in.attr, &out->attr)) return false;
    out->eol = in.eol;
    out->lifetimeA = in.lifetimeA;
    out->lifetimeB = in.lifetimeB;
    out->version = in.version;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::DiskStats& in,
        aidl::android::hardware::health::DiskStats* out) {
    out->reads = static_cast<int64_t>(in.reads);
    out->readMerges = static_cast<int64_t>(in.readMerges);
    out->readSectors = static_cast<int64_t>(in.readSectors);
    out->readTicks = static_cast<int64_t>(in.readTicks);
    out->writes = static_cast<int64_t>(in.writes);
    out->writeMerges = static_cast<int64_t>(in.writeMerges);
    out->writeSectors = static_cast<int64_t>(in.writeSectors);
    out->writeTicks = static_cast<int64_t>(in.writeTicks);
    out->ioInFlight = static_cast<int64_t>(in.ioInFlight);
    out->ioTicks = static_cast<int64_t>(in.ioTicks);
    out->ioInQueue = static_cast<int64_t>(in.ioInQueue);
    if (!translate(in.attr, &out->attr)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_1::HealthInfo& in,
        aidl::android::hardware::health::HealthInfo* out) {
    out->chargerAcOnline = static_cast<bool>(in.legacy.legacy.chargerAcOnline);
    out->chargerUsbOnline = static_cast<bool>(in.legacy.legacy.chargerUsbOnline);
    out->chargerWirelessOnline = static_cast<bool>(in.legacy.legacy.chargerWirelessOnline);
    out->maxChargingCurrent = static_cast<int32_t>(in.legacy.legacy.maxChargingCurrent);
    out->maxChargingVoltage = static_cast<int32_t>(in.legacy.legacy.maxChargingVoltage);
    out->batteryStatus = static_cast<aidl::android::hardware::health::BatteryStatus>(
            in.legacy.legacy.batteryStatus);
    out->batteryHealth = static_cast<aidl::android::hardware::health::BatteryHealth>(
            in.legacy.legacy.batteryHealth);
    out->batteryPresent = static_cast<bool>(in.legacy.legacy.batteryPresent);
    out->batteryLevel = static_cast<int32_t>(in.legacy.legacy.batteryLevel);
    out->batteryVoltage = static_cast<int32_t>(in.legacy.legacy.batteryVoltage);
    out->batteryTemperature = static_cast<int32_t>(in.legacy.legacy.batteryTemperature);
    out->batteryCurrent = static_cast<int32_t>(in.legacy.legacy.batteryCurrent);
    out->batteryCycleCount = static_cast<int32_t>(in.legacy.legacy.batteryCycleCount);
    out->batteryFullCharge = static_cast<int32_t>(in.legacy.legacy.batteryFullCharge);
    out->batteryChargeCounter = static_cast<int32_t>(in.legacy.legacy.batteryChargeCounter);
    out->batteryTechnology = in.legacy.legacy.batteryTechnology;
    out->batteryCurrentAverage = static_cast<int32_t>(in.legacy.batteryCurrentAverage);
    out->diskStats.clear();
    out->diskStats.resize(in.legacy.diskStats.size());
    for (size_t i = 0; i < in.legacy.diskStats.size(); ++i)
        if (!translate(in.legacy.diskStats[i], &out->diskStats[i])) return false;
    out->storageInfos.clear();
    out->storageInfos.resize(in.legacy.storageInfos.size());
    for (size_t i = 0; i < in.legacy.storageInfos.size(); ++i)
        if (!translate(in.legacy.storageInfos[i], &out->storageInfos[i])) return false;
    out->batteryCapacityLevel = static_cast<aidl::android::hardware::health::BatteryCapacityLevel>(
            in.batteryCapacityLevel);
    out->batteryChargeTimeToFullNowSeconds =
            static_cast<int64_t>(in.batteryChargeTimeToFullNowSeconds);
    out->batteryFullChargeDesignCapacityUah =
            static_cast<int32_t>(in.batteryFullChargeDesignCapacityUah);
    return true;
}

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_1::HealthConfig& in,
        aidl::android::hardware::health::HealthConfig* out) {
    out->periodicChoresIntervalFast = static_cast<int32_t>(in.battery.periodicChoresIntervalFast);
    out->periodicChoresIntervalSlow = static_cast<int32_t>(in.battery.periodicChoresIntervalSlow);
    out->batteryStatusPath = in.battery.batteryStatusPath;
    out->batteryHealthPath = in.battery.batteryHealthPath;
    out->batteryPresentPath = in.battery.batteryPresentPath;
    out->batteryCapacityPath = in.battery.batteryCapacityPath;
    out->batteryVoltagePath = in.battery.batteryVoltagePath;
    out->batteryTemperaturePath = in.battery.batteryTemperaturePath;
    out->batteryTechnologyPath = in.battery.batteryTechnologyPath;
    out->batteryCurrentNowPath = in.battery.batteryCurrentNowPath;
    out->batteryCurrentAvgPath = in.battery.batteryCurrentAvgPath;
    out->batteryChargeCounterPath = in.battery.batteryChargeCounterPath;
    out->batteryFullChargePath = in.battery.batteryFullChargePath;
    out->batteryCycleCountPath = in.battery.batteryCycleCountPath;
    out->bootMinCap = static_cast<int32_t>(in.bootMinCap);
    return true;
}

}  // namespace android::h2a
