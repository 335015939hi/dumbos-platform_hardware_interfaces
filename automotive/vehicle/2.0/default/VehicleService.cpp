/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "automotive.vehicle@2.0-service"
#include <android/log.h>
#include <hidl/HidlTransportSupport.h>

#include <iostream>

#include <vhal_v2_0/EmulatedUserHal.h>
#include <vhal_v2_0/EmulatedVehicleConnector.h>
#include <vhal_v2_0/EmulatedVehicleHal.h>
#include <vhal_v2_0/VehicleHalManager.h>

using namespace android;
using namespace android::hardware;
using namespace android::hardware::automotive::vehicle::V2_0;

int main(int /* argc */, char* /* argv */ []) {
    auto store = std::make_unique<VehiclePropertyStore>();
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    auto connector = std::make_unique<impl::EmulatedVehicleConnector>();
    auto userHal = connector->getEmulatedUserHal();
    auto hal = std::make_unique<impl::EmulatedVehicleHal>(store.get(), connector.get(), userHal);
    auto emulator = std::make_unique<impl::VehicleEmulator>(hal.get());
    auto service = std::make_unique<VehicleHalManager>(hal.get());
||||||| BASE
    auto connector = std::make_unique<DefaultVehicleConnector>();
    auto hal = std::make_unique<DefaultVehicleHal>(store.get(), connector.get());
    auto service = std::make_unique<VehicleHalManager>(hal.get());
=======
    auto connector = std::make_unique<DefaultVehicleConnector>();
    auto hal = std::make_unique<DefaultVehicleHal>(store.get(), connector.get());
    auto service = android::sp<VehicleHalManager>::make(hal.get());
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    connector->setValuePool(hal->getValuePool());

    configureRpcThreadpool(4, true /* callerWillJoin */);

    ALOGI("Registering as service...");
    status_t status = service->registerAsService();

    if (status != OK) {
        ALOGE("Unable to register vehicle service (%d)", status);
        return 1;
    }

    ALOGI("Ready");
    joinRpcThreadpool();

    return 1;
}
