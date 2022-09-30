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

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#define LOG_TAG "thermal_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/thermal/BnThermal.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <gtest/gtest.h>

#include <unistd.h>

namespace aidl::android::hardware::thermal {

namespace {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

using android::sp;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::thermal::CoolingDevice;
using android::hardware::thermal::CpuUsage;
using android::hardware::thermal::IThermal;
using android::hardware::thermal::Temperature;
using android::hardware::thermal::TemperatureType;
using android::hardware::thermal::ThermalStatus;
using android::hardware::thermal::ThermalStatusCode;

#define MONITORING_OPERATION_NUMBER 10

#define MAX_DEVICE_TEMPERATURE 200
#define MAX_FAN_SPEED 20000

// The main test class for THERMAL HIDL HAL.
class ThermalAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        thermal = IPower::fromBinder(ndk::SpAIBinder(binder));
        baseSize = 0;
        names.clear();
    }

    void TearDown() override {}

  protected:
    // Check validity of temperatures returned by Thremal HAL.
    void checkTemperatures(const std::vector<Temperature> temperatures) {
        size_t size = temperatures.size();
        EXPECT_LE(baseSize, size);

        for (size_t i = 0; i < size; ++i) {
            checkDeviceTemperature(temperatures[i]);
            if (i < baseSize) {
                EXPECT_EQ(names[i], temperatures[i].name.c_str());
            } else {
                // Names must be unique only for known temperature types.
                if (temperatures[i].type != TemperatureType::UNKNOWN) {
                    EXPECT_EQ(names.end(),
                              std::find(names.begin(), names.end(), temperatures[i].name.c_str()));
                }
                names.push_back(temperatures[i].name);
            }
        }
        baseSize = size;
    }

    // Check validity of CPU usages returned by Thermal HAL.
    void checkCpuUsages(const std::vector<CpuUsage>& cpuUsages) {
        size_t size = cpuUsages.size();
        // A number of CPU's does not change.
        if (baseSize != 0) {
            EXPECT_EQ(baseSize_, size);
        }

        for (size_t i = 0; i < size; ++i) {
            checkCpuUsage(cpuUsages[i]);
            if (i < baseSize) {
                EXPECT_EQ(names[i], cpuUsages[i].name.c_str());
            } else {
                // Names are not guaranteed to be unique because of the current
                // default Thermal HAL implementation.
                names.push_back(cpuUsages[i].name);
            }
        }
        baseSize = size;
    }

    // Check validity of cooling devices information returned by Thermal HAL.
    void checkCoolingDevices(const std::vector<CoolingDevice> coolingDevices) {
        size_t size = coolingDevices.size();
        EXPECT_LE(baseSize, size);

        for (size_t i = 0; i < size; ++i) {
            checkCoolingDevice(coolingDevices[i]);
            if (i < baseSize) {
                EXPECT_EQ(names_[i], coolingDevices[i].name.c_str());
            } else {
                // Names must be unique.
                EXPECT_EQ(names.end(),
                          std::find(names.begin(), names.end(), coolingDevices[i].name.c_str()));
                names.push_back(coolingDevices[i].name);
            }
        }
        baseSize = size;
    }

    std::shared_ptr<IThermal> thermal;

  private:
    // Check validity of temperature returned by Thermal HAL.
    void checkDeviceTemperature(const Temperature& temperature) {
        // .currentValue of known type is in Celsius and must be reasonable.
        EXPECT_TRUE(temperature.type == TemperatureType::UNKNOWN ||
                    std::abs(temperature.currentValue) < MAX_DEVICE_TEMPERATURE ||
                    isnan(temperature.currentValue));

        // .name must not be empty.
        EXPECT_LT(0u, temperature.name.size());

        // .currentValue must not exceed .shutdwonThreshold if defined.
        EXPECT_TRUE(temperature.currentValue < temperature.shutdownThreshold ||
                    isnan(temperature.currentValue) || isnan(temperature.shutdownThreshold));

        // .throttlingThreshold must not exceed .shutdownThreshold if defined.
        EXPECT_TRUE(temperature.throttlingThreshold < temperature.shutdownThreshold ||
                    isnan(temperature.throttlingThreshold) || isnan(temperature.shutdownThreshold));
    }

    // Check validity of CPU usage returned by Thermal HAL.
    void checkCpuUsage(const CpuUsage& cpuUsage) {
        // .active must be less than .total if CPU is online.
        EXPECT_TRUE(!cpuUsage.isOnline || (cpuUsage.active >= 0 && cpuUsage.total >= 0 &&
                                           cpuUsage.total >= cpuUsage.active));

        // .name must be not empty.
        EXPECT_LT(0u, cpuUsage.name.size());
    }

    // Check validity of a cooling device information returned by Thermal HAL.
    void checkCoolingDevice(const CoolingDevice& coolingDevice) {
        EXPECT_LE(0, coolingDevice.currentValue);
        EXPECT_GT(MAX_FAN_SPEED, coolingDevice.currentValue);
        EXPECT_LT(0u, coolingDevice.name.size());
    }

    size_t baseSize;
    std::vector<std::string> names;
};

// Sanity test for Thermal::getTemperatures().
TEST_P(ThermalAidlTest, TemperatureTest) {
    std::vector<Temperature> passed;
    for (size_t i = 0; i < MONITORING_OPERATION_NUMBER; ++i) {
        thermal->getTemperatures(
                [&passed](ThermalStatus status, std::vector<Temperature> temperatures) {
                    EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                    passed = temperatures;
                });

        checkTemperatures(passed);
        sleep(1);
    }
}

/*
// Sanity test for Thermal::getCpuUsages().
TEST_P(ThermalHidlTest, CpuUsageTest) {
  hidl_vec<CpuUsage> passed;
  for (size_t i = 0; i < MONITORING_OPERATION_NUMBER; ++i) {
    thermal_->getCpuUsages(
        [&passed](ThermalStatus status, hidl_vec<CpuUsage> cpuUsages) {
          EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
          passed = cpuUsages;
        });

    checkCpuUsages(passed);
    sleep(1);
  }
}

// Sanity test for Thermal::getCoolingDevices().
TEST_P(ThermalHidlTest, CoolingDeviceTest) {
  hidl_vec<CoolingDevice> passed;
  for (size_t i = 0; i < MONITORING_OPERATION_NUMBER; ++i) {
    thermal_->getCoolingDevices([&passed](
        ThermalStatus status, hidl_vec<CoolingDevice> coolingDevices) {
      EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
      passed = coolingDevices;
    });

    checkCoolingDevices(passed);
    sleep(1);
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ThermalAidlTest);
INSTANTIATE_TEST_SUITE_P(Thermal, ThermalAidlTest,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(IThermal::descriptor)),
                         ::android::PrintInstanceNameToString);

        */
}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}

}  // namespace aidl::android::hardware::thermal
