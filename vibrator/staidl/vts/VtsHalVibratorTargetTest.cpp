/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <aidl/GtestPrinter.h>
#include <aidl/Vintf.h>

#include <android/hardware/vibrator/IVibrator.h>
#include <binder/IServiceManager.h>

using android::sp;
using android::String16;
using android::hardware::vibrator::IVibrator;

class VibratorAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        vibrator = android::waitForDeclaredService<IVibrator>(String16(GetParam().c_str()));
        ASSERT_NE(vibrator, nullptr);
    }

  private:
    sp<IVibrator> vibrator;
};

TEST_P(VibratorAidl, Example) {}

INSTANTIATE_TEST_SUITE_P(, VibratorAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IVibrator::descriptor)),
                         android::PrintInstanceNameToString);
