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

#include <health-impl/health-convert.h>

namespace aidl::android::hardware::health {

void convert(const HealthInfo& info, struct ::android::BatteryProperties* p) {
    p->chargerAcOnline = info.chargerAcOnline;
    p->chargerUsbOnline = info.chargerUsbOnline;
    p->chargerWirelessOnline = info.chargerWirelessOnline;
    p->maxChargingCurrent = info.maxChargingCurrent;
    p->maxChargingVoltage = info.maxChargingVoltage;
    p->batteryStatus = static_cast<int>(info.batteryStatus);
    p->batteryHealth = static_cast<int>(info.batteryHealth);
    p->batteryPresent = info.batteryPresent;
    p->batteryLevel = info.batteryLevel;
    p->batteryVoltage = info.batteryVoltage;
    p->batteryTemperature = info.batteryTemperature;
    p->batteryCurrent = info.batteryCurrent;
    p->batteryCycleCount = info.batteryCycleCount;
    p->batteryFullCharge = info.batteryFullCharge;
    p->batteryChargeCounter = info.batteryChargeCounter;
    p->batteryTechnology = ::android::String8(info.batteryTechnology.c_str());
}

void convert(const struct healthd_config* hc, HealthConfig* config) {
    config->periodicChoresIntervalFast = hc->periodic_chores_interval_fast;
    config->periodicChoresIntervalSlow = hc->periodic_chores_interval_slow;

    config->batteryStatusPath = hc->batteryStatusPath.string();
    config->batteryHealthPath = hc->batteryHealthPath.string();
    config->batteryPresentPath = hc->batteryPresentPath.string();
    config->batteryCapacityPath = hc->batteryCapacityPath.string();
    config->batteryVoltagePath = hc->batteryVoltagePath.string();
    config->batteryTemperaturePath = hc->batteryTemperaturePath.string();
    config->batteryTechnologyPath = hc->batteryTechnologyPath.string();
    config->batteryCurrentNowPath = hc->batteryCurrentNowPath.string();
    config->batteryCurrentAvgPath = hc->batteryCurrentAvgPath.string();
    config->batteryChargeCounterPath = hc->batteryChargeCounterPath.string();
    config->batteryFullChargePath = hc->batteryFullChargePath.string();
    config->batteryCycleCountPath = hc->batteryCycleCountPath.string();

    config->bootMinCap = static_cast<int32_t>(hc->boot_min_cap);
}

void convert(const HealthConfig& c, struct healthd_config* hc) {
    hc->periodic_chores_interval_fast = c.periodicChoresIntervalFast;
    hc->periodic_chores_interval_slow = c.periodicChoresIntervalSlow;
    hc->batteryStatusPath =
            ::android::String8(c.batteryStatusPath.c_str(), c.batteryStatusPath.size());
    hc->batteryHealthPath =
            ::android::String8(c.batteryHealthPath.c_str(), c.batteryHealthPath.size());
    hc->batteryPresentPath =
            ::android::String8(c.batteryPresentPath.c_str(), c.batteryPresentPath.size());
    hc->batteryCapacityPath =
            ::android::String8(c.batteryCapacityPath.c_str(), c.batteryCapacityPath.size());
    hc->batteryVoltagePath =
            ::android::String8(c.batteryVoltagePath.c_str(), c.batteryVoltagePath.size());
    hc->batteryTemperaturePath =
            ::android::String8(c.batteryTemperaturePath.c_str(), c.batteryTemperaturePath.size());
    hc->batteryTechnologyPath =
            ::android::String8(c.batteryTechnologyPath.c_str(), c.batteryTechnologyPath.size());
    hc->batteryCurrentNowPath =
            ::android::String8(c.batteryCurrentNowPath.c_str(), c.batteryCurrentNowPath.size());
    hc->batteryCurrentAvgPath =
            ::android::String8(c.batteryCurrentAvgPath.c_str(), c.batteryCurrentAvgPath.size());
    hc->batteryChargeCounterPath = ::android::String8(c.batteryChargeCounterPath.c_str(),
                                                      c.batteryChargeCounterPath.size());
    hc->batteryFullChargePath =
            ::android::String8(c.batteryFullChargePath.c_str(), c.batteryFullChargePath.size());
    hc->batteryCycleCountPath =
            ::android::String8(c.batteryCycleCountPath.c_str(), c.batteryCycleCountPath.size());

    hc->boot_min_cap = static_cast<int>(c.bootMinCap);

    // energyCounter / screen_on is handled through special means so all calls to
    // the function go across the HALs

    // ignorePowerSupplyNames - not used by clients of health HAL. The common health HAL impl
    // does not use this variable.
}

}  // namespace aidl::android::hardware::health
