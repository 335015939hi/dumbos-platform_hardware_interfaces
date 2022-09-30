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

namespace aidl::android::hardware::thermal::impl::example {

ndk::ScopedAStatus Thermal::getCoolingDevices(ThermalStatus* out_status,
                                              std::vector<CoolingDevice>* out_devices) {
    LOG(VERBOSE) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus getCpuUsages(ThermalStatus* out_status, std::vector<CpuUsage>* out_cpuUsages) {
    LOG(VERBOSE) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus getCurrentCoolingDevices(bool in_filterType, CoolingType in_type,
                                            ThermalStatus* out_status,
                                            std::vector<CoolingDevice>* out_devices) {
    LOG(VERBOSE) << __func__ << " filterType: " << in_filterType << ", CoolingType: " << in_type;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus getCurrentTemperatures(bool in_filterType, TemperatureType in_type,
                                          ThermalStatus* out_status,
                                          std::vector<Temperature>* out_temperatures) {
    LOG(VERBOSE) << __func__ << " filterType: " << in_filterType
                 << ", TemperatureType: " << in_type;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus getTemperatureThresholds(
        bool in_filterType, TemperatureType in_type, ThermalStatus* out_status,
        std::vector<TemperatureThreshold>* out_temperatureThresholds) {
    LOG(VERBOSE) << __func__ << " filterType: " << in_filterType
                 << ", TemperatureType: " << in_type;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus getTemperatures(ThermalStatus* out_status,
                                   std::vector<Temperature>* out_temperatures) {
    LOG(VERBOSE) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus registerThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback, bool in_filterType,
        TemperatureType in_type, ThermalStatus* _aidl_return) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback
                 << ", filterType: " << in_filterType << ", TemperatureType: " << in_type;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus unregisterThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback, ThermalStatus* _aidl_return) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::thermal::impl::example
