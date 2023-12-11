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
#pragma once

#include <android-base/logging.h>

namespace android::hardware::radio::minimal {

namespace debug {

static constexpr bool kSuperVerbose = true;

// clang-format off
#define LOG_CALL                                                             \
    if constexpr (::android::hardware::radio::minimal::debug::kSuperVerbose) \
        LOG(VERBOSE) << '[' << serial << ("] " RADIO_MODULE ".") << __func__ << ' '

#define LOG_CALL_NOSERIAL                                                    \
    if constexpr (::android::hardware::radio::minimal::debug::kSuperVerbose) \
        LOG(VERBOSE) << (RADIO_MODULE ".") << __func__ << ' '
// clang-format on

#define LOG_NOT_IMPLEMENTED \
    LOG(WARNING) << '[' << serial << ("] Not implemented: " RADIO_MODULE ".") << __func__ << ' '

}  // namespace debug

}  // namespace android::hardware::radio::minimal
