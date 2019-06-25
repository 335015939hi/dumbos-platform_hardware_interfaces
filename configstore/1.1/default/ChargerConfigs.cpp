/*
 * Copyright (C) 2019 The Android Open Source Project
 * Copyright (C) 2019 Felix Elsner
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

#include "ChargerConfigs.h"

// TODO: Or rather 1.0/types.h? Are 1.0 types imported
// to 1.1 in C/C++?
#include <android/hardware/configstore/1.1/types.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_1 {
namespace implementation {

// Methods from ::android::hardware::configstore::V1_1::IChargerConfigs follow.
Return<void> ChargerConfigs::drawSplitScreen(drawSplitScreen_cb _hidl_cb) {
#ifdef HEALTHD_DRAW_SPLIT_SCREEN
    _hidl_cb({true, HEALTHD_DRAW_SPLIT_SCREEN});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> ChargerConfigs::drawSplitOffset(drawSplitOffset_cb _hidl_cb) {
#ifdef HEALTHD_DRAW_SPLIT_OFFSET
    _hidl_cb({true, HEALTHD_DRAW_SPLIT_SCREEN});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> ChargerConfigs::disableInitBlank(disableInitBlank_cb _hidl_cb) {
    bool value = false;
#ifdef CHARGER_DISABLE_INIT_BLANK
    value = true;
    _hidl_cb({true, value});
#else
    _hidl_cb({false, value});
#endif
    return Void();
}

Return<void> ChargerConfigs::enableSuspend(enableSuspend_cb _hidl_cb) {
    bool value = false;
#ifdef CHARGER_ENABLE_SUSPEND
    value = true;
    _hidl_cb({true, value});
#else
    _hidl_cb({false, value});
#endif
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace configstore
}  // namespace hardware
}  // namespace android
