/*
 * Copyright 2018 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACE_KEYMASTER_CAPABILITY_UTILS_H_
#define HARDWARE_INTERFACE_KEYMASTER_CAPABILITY_UTILS_H_

#include <android/hardware/keymaster/capability/1.0/types.h>

::android::hardware::keymaster::capability::V1_0::KeymasterCapability hidlVec2KeymasterCapability(
    ::android::hardware::hidl_vec<uint8_t> token);

#endif  // HARDWARE_INTERFACE_KEYMASTER_CAPABILITY_UTILS_H_
