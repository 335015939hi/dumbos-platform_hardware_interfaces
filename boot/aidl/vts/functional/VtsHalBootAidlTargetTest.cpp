/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <android-base/logging.h>

#include <cutils/properties.h>

#include <aidl/android/hardware/boot/IBootControl.h>

#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unordered_set>

using aidl::android::hardware::boot::IBootControl;
using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using std::string;
using std::unordered_set;
using std::vector;

// The main test class for the Boot HIDL HAL.
class BootHidlTest : public ::testing::Test {
  public:
    virtual void SetUp() override {
        const auto instance_name = std::string(IBootControl::descriptor) + "/default";
        ASSERT_TRUE(AServiceManager_isDeclared(instance_name.c_str()));
        boot = ::aidl::android::hardware::boot::IBootControl::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance_name.c_str())));
        ASSERT_NE(boot, nullptr);
    }

    virtual void TearDown() override {}

    std::shared_ptr<IBootControl> boot;
};

// Sanity check Boot::getNumberSlots().
TEST_F(BootHidlTest, GetNumberSlots) {
    int32_t slots{};
    boot->getNumberSlots(&slots);
    ASSERT_LE(2, slots);
}

// Sanity check Boot::getCurrentSlot().
TEST_F(BootHidlTest, GetCurrentSlot) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    int slots = 0;
    boot->getNumberSlots(&slots);
    ASSERT_LT(curSlot, slots);
}

// Sanity check Boot::markBootSuccessful().
TEST_F(BootHidlTest, MarkBootSuccessful) {
    const auto result = boot->markBootSuccessful();
    ASSERT_TRUE(result.isOk());
    int curSlot = 0;
    boot->getCurrentSlot(&curSlot);
    bool ret = false;
    boot->isSlotMarkedSuccessful(curSlot, &ret);
    ASSERT_TRUE(ret);
}

TEST_F(BootHidlTest, SetActiveBootSlot) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    ASSERT_GE(curSlot, 0);
    int otherSlot = curSlot ? 0 : 1;
    bool otherBootable = true;
    boot->isSlotBootable(otherSlot, &otherBootable);

    for (int s = 0; s < 2; s++) {
        const auto result = boot->setActiveBootSlot(s);
        ASSERT_TRUE(result.isOk());
    }
    {
        // Restore original flags to avoid problems on reboot
        auto result = boot->setActiveBootSlot(curSlot);
        ASSERT_TRUE(result.isOk());

        if (!otherBootable) {
            const auto result = boot->setSlotAsUnbootable(otherSlot);
            ASSERT_TRUE(result.isOk());
        }

        result = boot->markBootSuccessful();
        ASSERT_TRUE(result.isOk());
    }
    {
        int slots = 0;
        boot->getNumberSlots(&slots);
        const auto result = boot->setActiveBootSlot(slots);
        ASSERT_FALSE(result.isOk()) << "setActiveBootSlot on invalid slot should fail";
    }
}

TEST_F(BootHidlTest, SetSlotAsUnbootable) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    ASSERT_GE(curSlot, 0);
    int otherSlot = curSlot ? 0 : 1;
    bool otherBootable = false;
    boot->isSlotBootable(otherSlot, &otherBootable);
    {
        auto result = boot->setSlotAsUnbootable(otherSlot);
        ASSERT_TRUE(result.isOk());
        boot->isSlotBootable(otherSlot, &otherBootable);
        ASSERT_FALSE(otherBootable);

        // Restore original flags to avoid problems on reboot
        if (otherBootable) {
            result = boot->setActiveBootSlot(otherSlot);
            ASSERT_TRUE(result.isOk());
        }
        result = boot->setActiveBootSlot(curSlot);
        ASSERT_TRUE(result.isOk());
        result = boot->markBootSuccessful();
        ASSERT_TRUE(result.isOk());
    }
    {
        int32_t slots = 0;
        boot->getNumberSlots(&slots);
        const auto result = boot->setSlotAsUnbootable(slots);
        ASSERT_FALSE(result.isOk());
    }
}

// Sanity check Boot::isSlotBootable() on good and bad inputs.
TEST_F(BootHidlTest, IsSlotBootable) {
    for (int s = 0; s < 2; s++) {
        bool bootable = false;
        const auto res = boot->isSlotBootable(s, &bootable);
        ASSERT_TRUE(res.isOk()) << res.getMessage();
    }
    int32_t slots = 0;
    boot->getNumberSlots(&slots);
    bool bootable = false;
    const auto res = boot->isSlotBootable(slots, &bootable);
    ASSERT_FALSE(res.isOk());
}

// Sanity check Boot::isSlotMarkedSuccessful() on good and bad inputs.
TEST_F(BootHidlTest, IsSlotMarkedSuccessful) {
    for (int32_t s = 0; s < 2; s++) {
        bool isSuccess = false;
        const auto res = boot->isSlotMarkedSuccessful(s, &isSuccess);
    }
    int32_t slots = 0;
    boot->getNumberSlots(&slots);
    bool isSuccess = false;
    const auto res = boot->isSlotMarkedSuccessful(slots, &isSuccess);
    ASSERT_FALSE(res.isOk());
}

// Sanity check Boot::getSuffix() on good and bad inputs.
TEST_F(BootHidlTest, GetSuffix) {
    string suffixStr;
    unordered_set<string> suffixes;
    int numSlots = 0;
    boot->getNumberSlots(&numSlots);
    for (int32_t i = 0; i < numSlots; i++) {
        std::string suffix;
        const auto result = boot->getSuffix(i, &suffixStr);
        ASSERT_TRUE(result.isOk());
        ASSERT_EQ('_', suffixStr[0]);
        ASSERT_LE((unsigned)2, suffixStr.size());
        suffixes.insert(suffixStr);
    }
    // All suffixes should be unique
    ASSERT_EQ(numSlots, suffixes.size());
    {
        const string emptySuffix = "";
        const auto result = boot->getSuffix(numSlots, &suffixStr);
        ASSERT_TRUE(result.isOk());
        ASSERT_EQ(suffixStr, emptySuffix);
    }
}
