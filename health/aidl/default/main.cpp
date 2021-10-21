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

#include "health-impl/Health.h"

#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <charger.sysprop.h>
#include <health/utils.h>

#ifndef CHARGER_FORCE_NO_UI
#define CHARGER_FORCE_NO_UI 0
#endif

using aidl::android::hardware::health::Health;

static constexpr const char* gInstanceName = "default";
static constexpr std::string_view gChargerArg{"--charger"};

namespace {
int charger_nops() {
    // FIXME
    //    HalHealthLoop charger("charger", GetHealthServiceOrDefault());
    //    return charger.StartLoop();
    return 0;
}

int real_charger_main() {
    // FIXME
    //    android::ChargerHidl charger(GetHealthServiceOrDefault());
    //    return charger.StartLoop();
    return 0;
}

int charger_main(char** argv) {
    android::base::InitLogging(argv, &android::base::KernelLogger);
    if (CHARGER_FORCE_NO_UI || android::sysprop::ChargerProperties::no_ui().value_or(false)) {
        return charger_nops();
    } else {
        return real_charger_main();
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && argv[1] == gChargerArg) {
        return charger_main(argv);
    }

    // make a default health service
    auto config = std::make_unique<healthd_config>();
    ::android::hardware::health::InitHealthdConfig(config.get());

    auto binder = ndk::SharedRefBase::make<Health>(gInstanceName, std::move(config));
    return binder->StartLoop();
}
