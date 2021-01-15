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

#include <OffloadControlTestBaseV1_1.h>

void OffloadControlTestBase::prepareControlHal() {
    control = createControl(std::get<1>(GetParam()));
    ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

    control_cb_1_1 = new TetheringOffloadCallback();
    ASSERT_NE(nullptr, control_cb.get()) << "Could not get get offload callback";
}

void OffloadControlTestBase::initOffload(const bool expected_result) {
    auto init_cb = [&](bool success, std::string errMsg) {
        std::string msg = StringPrintf("Unexpectedly %s to init offload: %s",
                                       success ? "succeeded" : "failed", errMsg.c_str());
        ASSERT_EQ(expected_result, success) << msg;
    };
    const Return<void> ret = control->initOffload_1_1(control_cb_1_1, init_cb);
    ASSERT_TRUE(ret.isOk());
}
