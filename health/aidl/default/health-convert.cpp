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

}  // namespace aidl::android::hardware::health
