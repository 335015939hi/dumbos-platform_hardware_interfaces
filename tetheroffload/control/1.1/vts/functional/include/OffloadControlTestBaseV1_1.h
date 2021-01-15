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

#pragma once

#include <OffloadControlTestV1_0.h>
#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.1/ITetheringOffloadCallback.h>

constexpr char kCallbackOnEvent_1_1[] = "onEvent_1_1";

class TetheringOffloadCallbackArgsV1_1 {
  public:
    android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent last_event;
};

class OffloadControlTestBaseV1_1 : public OffloadControlTestBase {
  public:
    void initOffload(const bool expected_result);

    // Callback class for both events and NAT timeout updates.
    class TetheringOffloadCallbackV1_1
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgsV1_1>,
          public android::hardware::tetheroffload::control::V1_1::ITetheringOffloadCallback {
      public:
        Return<void> onEvent_1_1(
                    android::hardware::tetheroffload::control::V1_1::OffloadCallbackEvent event)
                            override {
            const TetheringOffloadCallbackArgsV1_1 args{.last_event = event};
            NotifyFromCallback(kCallbackOnEvent_1_1, args);
            return Void();
        };
    };

    sp<TetheringOffloadCallbackV1_1> control_cb;
};
