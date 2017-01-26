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

#include <hidl/Status.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "wifi_hidl_call_util.h"

namespace {
class Dummy {};

class IExample : public ::android::RefBase {
   public:
    using startWithCallbackCopy_cb = std::function<void(int)>;
    using startWithCallbackReference_cb = std::function<void(int)>;
    static constexpr int kByCopyResult = 42;
    static constexpr int kByReferenceResult = 420;

    ::android::hardware::Return<void> startWithCallbackCopy(
        startWithCallbackCopy_cb _hidl_cb) {
        _hidl_cb(kByCopyResult);
        return ::android::hardware::Void();
    }
    ::android::hardware::Return<void> startWithCallbackReference(
        const startWithCallbackReference_cb& _hidl_cb) {
        _hidl_cb(kByReferenceResult);
        return ::android::hardware::Void();
    }
};

constexpr int IExample::kByCopyResult;
constexpr int IExample::kByReferenceResult;
}  // namespace

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

static_assert(std::is_same<Dummy, detail::functionArgSaver<std::function<
                                      void(const Dummy&)>>::StorageT>::value,
              "Reference should be stored by copy.");

TEST(HidlInvokeTest, WorksWithMethodThatTakesResultCallbackByValue) {
    ::android::sp<IExample> sp = new IExample();
    EXPECT_EQ(IExample::kByCopyResult, HIDL_INVOKE(sp, startWithCallbackCopy));
}

TEST(HidlInvokeTest, WorksWithMethodThatTakesResultCallbackByConstReference) {
    ::android::sp<IExample> sp = new IExample();
    EXPECT_EQ(IExample::kByReferenceResult,
              HIDL_INVOKE(sp, startWithCallbackReference));
}
