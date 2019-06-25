/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.configstore@1.1-service"

#include <android/hardware/configstore/1.1/ISurfaceFlingerConfigs.h>
#include <android/hardware/configstore/1.1/IChargerConfigs.h>
#include <hidl/HidlTransportSupport.h>
#include <hwminijail/HardwareMinijail.h>

#include "SurfaceFlingerConfigs.h"
#include "ChargerConfigs.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::configstore::V1_1::ISurfaceFlingerConfigs;
using android::hardware::configstore::V1_1::IChargerConfigs;
using android::hardware::configstore::V1_1::implementation::SurfaceFlingerConfigs;
using android::hardware::configstore::V1_1::implementation::ChargerConfigs;
using android::hardware::SetupMinijail;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(10, true);

    SetupMinijail("/vendor/etc/seccomp_policy/configstore@1.1.policy");

    status_t status = 0;

    sp<ISurfaceFlingerConfigs> surfaceFlingerConfigs = new SurfaceFlingerConfigs;
    status = surfaceFlingerConfigs->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register ISurfaceFlingerConfigs");

    sp<IChargerConfigs> chargerConfigs = new ChargerConfigs;
    status = chargerConfigs->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register ChargerConfigs");

    // other interface registration comes here
    joinRpcThreadpool();
    return 0;
}
