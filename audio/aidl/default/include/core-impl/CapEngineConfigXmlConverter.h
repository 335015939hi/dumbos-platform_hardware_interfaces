/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <aidl/android/hardware/audio/core/BnConfig.h>
#include <system/audio_config.h>

#include "EngineConfigXmlConverter.h"

namespace aidl::android::hardware::audio::core::internal {

class CapEngineConfigXmlConverter {

public:
    static ::aidl::android::media::audio::common::AudioHalCapDomains getAidlCapEngineConfig();

private:
    /** Default path of audio policy cap engine configuration file. */
    static constexpr char kCapEngineConfigFileName[] =
            "vendor/etc/parameter-framework/Settings/Policy/PolicyConfigurableDomains.xml";
};
}