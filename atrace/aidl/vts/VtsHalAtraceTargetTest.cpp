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

#include <set>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android/hardware/atrace/IAtraceDevice.h>
#include <android/hardware/atrace/TracingCategory.h>
#include <android/hardware/atrace/TracingEvent.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::atrace::IAtraceDevice;
using android::hardware::atrace::TracingCategory;
using android::hardware::atrace::TracingEvent;

using testing::HasSubstr;
using testing::Not;

class AtraceDeviceAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        auto name = String16(GetParam().c_str());
        atrace = android::waitForDeclaredService<IAtraceDevice>(name);
        ASSERT_NE(atrace, nullptr);
    }

    sp<IAtraceDevice> atrace;
};

TEST_P(AtraceDeviceAidl, listCategories) {
    std::vector<TracingCategory> categories;
    ASSERT_TRUE(atrace->listCategories(&categories).isOk());

    std::set<String16> names;
    for (const auto& category : categories) {
        names.insert(category.name);

        // Names should not be empty:
        ASSERT_NE(category.name, String16(""));

        // Names should not contain spaces:
        ASSERT_EQ(category.name.findFirst(u' '), -1);

        // Description should not be empty:
        ASSERT_NE(category.description, String16(""));

        for (const TracingEvent& event : category.events) {
            ASSERT_NE(event.group, String16(""));
            ASSERT_NE(event.name, String16(""));
        }
    }

    // Each category should have a unique name:
    EXPECT_EQ(names.size(), categories.size());
}

TEST_P(AtraceDeviceAidl, enableCategories_emptyFails) {
    ASSERT_FALSE(atrace->enableCategories({}).isOk());
}

TEST_P(AtraceDeviceAidl, enableCategories_badEventFails) {
    ASSERT_FALSE(atrace->enableCategories({String16("notACategoryName")}).isOk());
}

TEST_P(AtraceDeviceAidl, enableCategories) {
    std::vector<TracingCategory> categories;
    EXPECT_TRUE(atrace->listCategories(&categories).isOk());
    for (const auto& category : categories) {
        EXPECT_TRUE(atrace->enableCategories({category.name}).isOk());
    }
    EXPECT_TRUE(atrace->disableAllCategories().isOk());
}

TEST_P(AtraceDeviceAidl, disableCategories) {
    ASSERT_TRUE(atrace->disableAllCategories().isOk());
}

INSTANTIATE_TEST_SUITE_P(
        AtraceDevice, AtraceDeviceAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IAtraceDevice::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
