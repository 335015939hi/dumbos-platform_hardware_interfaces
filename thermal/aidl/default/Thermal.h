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

#pragma once

#include <aidl/android/hardware/thermal/BnThermal.h>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {
namespace impl {
namespace example {

class Thermal : public BnThermal {
    ndk::ScopedAStatus getCoolingDevices(ThermalStatus* out_status,
                                         std::vector<CoolingDevice>* out_devices) override;

    ndk::ScopedAStatus getCpuUsages(ThermalStatus* out_status,
                                    std::vector<CpuUsage>* out_cpuUsages) override;

    ndk::ScopedAStatus getCurrentCoolingDevices(bool in_filterType, CoolingType in_type,
                                                ThermalStatus* out_status,
                                                std::vector<CoolingDevice>* out_devices) override;

    ndk::ScopedAStatus getCurrentTemperatures(bool in_filterType, TemperatureType in_type,
                                              ThermalStatus* out_status,
                                              std::vector<Temperature>* out_temperatures) override;

    ndk::ScopedAStatus getTemperatureThresholds(
            bool in_filterType, TemperatureType in_type, ThermalStatus* out_status,
            std::vector<TemperatureThreshold>* out_temperatureThresholds) override;

    ndk::ScopedAStatus getTemperatures(ThermalStatus* out_status,
                                       std::vector<Temperature>* out_temperatures) override;

    ndk::ScopedAStatus registerThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback, bool in_filterType,
            TemperatureType in_type, ThermalStatus* _aidl_return) override;

    ndk::ScopedAStatus unregisterThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback,
            ThermalStatus* _aidl_return) override;
};

}  // namespace example
}  // namespace impl
}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
