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

#include <string>
#include <vector>

#include <vintf/fcm_exclude.h>

namespace android::vintf {

// TODO(b/110261831): reduce items in these lists

std::vector<std::string> gFcmExcludedPrefixes{
        "android.hardware.gnss.measurement_corrections@",
        "android.hardware.graphics.bufferqueue@",

        // Exempted.
        "android.hardware.camera.device@",
        "android.hardware.tests.",
};

std::vector<std::string> gFcmExcludedExact{
        "android.hardware.audio@7.0",
        "android.hardware.audio.effect@7.0",
        "android.hardware.biometrics.fingerprint@2.3",
        "android.hardware.cas.native@1.0",
        "android.hardware.fastboot@1.0",
        "android.hardware.gnss.visibility_control@1.0",
        "android.hardware.media.bufferpool@1.0",
        "android.hardware.media.bufferpool@2.0",
        "android.hardware.radio.config@1.2",
        "android.hardware.tv.cec@2.0",
        "android.hardware.tv.tuner@1.0",
        "android.hardware.keymaster",

        // Exempted
        "android.hardware.common",
        "android.hardware.graphics.common",
};

}  // namespace android::vintf
