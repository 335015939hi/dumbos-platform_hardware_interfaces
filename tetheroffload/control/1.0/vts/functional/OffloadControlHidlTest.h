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

#include <OffloadControlHidlTestBase.h>

class OffloadControlHidlTest : public OffloadControlHidlTestBase {
  public:
    virtual void SetUp() override {
        setupConfigHal();
        setupControlHal();
    }

    virtual void TearDown() override {
        // For good measure, we should try stopOffload() once more. Since we
        // don't know where we are in HAL call test cycle we don't know what
        // return code to actually expect, so we just ignore it.
        stopOffload(ExpectBoolean::Ignored);
    }
};
