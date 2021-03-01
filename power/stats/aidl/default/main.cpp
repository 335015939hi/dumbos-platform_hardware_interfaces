/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "PowerStats.h"

#include "MockEnergyConsumer.h"
#include "MockEnergyMeter.h"
#include "MockStateResidencyDataProvider.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::power::stats::EnergyConsumerType;
using aidl::android::hardware::power::stats::MockEnergyConsumer;
using aidl::android::hardware::power::stats::MockEnergyMeter;
using aidl::android::hardware::power::stats::MockStateResidencyDataProvider;
using aidl::android::hardware::power::stats::PowerStats;
using aidl::android::hardware::power::stats::State;

void setExampleEnergyMeter(std::shared_ptr<PowerStats> p) {
    p->setEnergyMeter(
            std::make_unique<MockEnergyMeter>(std::vector<std::pair<std::string, std::string>>{
                    {"Rail1", "Display"},
                    {"Rail2", "CPU"},
                    {"Rail3", "Modem"},
            }));
}

void addExampleStateResidencyDataProvider1(std::shared_ptr<PowerStats> p) {
    p->addStateResidencyDataProvider(std::make_unique<MockStateResidencyDataProvider>(
            "CPU", std::vector<State>{{0, "Idle"}, {1, "Active"}}));
}

void addExampleStateResidencyDataProvider2(std::shared_ptr<PowerStats> p) {
    p->addStateResidencyDataProvider(std::make_unique<MockStateResidencyDataProvider>(
            "Display", std::vector<State>{{0, "Off"}, {1, "On"}}));
}

void addExampleEnergyConsumer1(std::shared_ptr<PowerStats> p) {
    p->addEnergyConsumer(std::make_unique<MockEnergyConsumer>(EnergyConsumerType::OTHER, "GPU"));
}

void addExampleEnergyConsumer2(std::shared_ptr<PowerStats> p) {
    p->addEnergyConsumer(
            std::make_unique<MockEnergyConsumer>(EnergyConsumerType::MOBILE_RADIO, "MODEM"));
}

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<PowerStats> p = ndk::SharedRefBase::make<PowerStats>();

    setExampleEnergyMeter(p);

    addExampleStateResidencyDataProvider1(p);
    addExampleStateResidencyDataProvider2(p);

    addExampleEnergyConsumer1(p);
    addExampleEnergyConsumer2(p);

    const std::string instance = std::string() + PowerStats::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(p->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
