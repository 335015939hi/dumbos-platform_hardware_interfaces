/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "SurfaceFlingerConfigs.h"

namespace android {
namespace hardware {
namespace configstore {
namespace V1_1 {
namespace implementation {

// Methods from ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs
// follow.
Return<void> SurfaceFlingerConfigs::vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
#ifdef VSYNC_EVENT_PHASE_OFFSET_NS
    _hidl_cb({true, VSYNC_EVENT_PHASE_OFFSET_NS});
#else
    _hidl_cb({false, 0});
#endif
    return Void();
}

Return<void> SurfaceFlingerConfigs::useTripleFramebuffer(useTripleFramebuffer_cb _hidl_cb) {
    bool value = false;
#ifdef USE_TRIPLE_FRAMEBUFFER
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

// Methods from ::android::hardware::configstore::V1_1::ISurfaceFlingerConfigs
// follow.

// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_1
}  // namespace configstore
}  // namespace hardware
}  // namespace android
