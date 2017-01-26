/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <functional>
#include <type_traits>

#include "wifi_hidl_call_util.h"

static_assert(std::is_same<int, detail::functionArgSaver<
                                    std::function<void(int)>>::StorageT>::value,
              "Single-arg result should be stored directly.");

static_assert(
    std::is_same<std::pair<int, long>, detail::functionArgSaver<std::function<
                                           void(int, long)>>::StorageT>::value,
    "Two-arg result should be stored as a pair.");

static_assert(
    std::is_same<std::tuple<char, int, long>,
                 detail::functionArgSaver<
                     std::function<void(char, int, long)>>::StorageT>::value,
    "Three-arg result should be stored as a tuple.");

struct Dummy {};

static_assert(
    std::is_same<struct Dummy, detail::functionArgSaver<std::function<void(
                                   const struct Dummy&)>>::StorageT>::value,
    "Reference should be stored by copy.");
