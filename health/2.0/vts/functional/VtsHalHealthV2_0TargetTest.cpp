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

#define LOG_TAG "health_hidl_hal_test"

#include <mutex>

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/health/2.0/IHealth.h>
#include <android/hardware/health/2.0/types.h>
#include <log/log.h>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::health::V1_0::BatteryStatus;
using ::android::hardware::health::V1_0::HealthInfo;
using ::android::hardware::health::V2_0::IHealth;
using ::android::hardware::health::V2_0::IHealthInfoCallback;
using ::android::hardware::health::V2_0::Result;
using ::android::hardware::health::V2_0::toString;

using ::android::sp;

using ::testing::AssertionFailure;
using ::testing::AssertionSuccess;

class HealthHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        health_ = ::testing::VtsHalHidlTargetTestBase::getService<IHealth>();
        ASSERT_NE(health_, nullptr);
    }

    sp<IHealth> health_;
};

class Callback : public IHealthInfoCallback {
    using Function = std::function<void(const HealthInfo&)>;

   public:
    Callback(const Function& f) : internal_(f) {}
    Return<void> healthInfoChanged(const HealthInfo& info) override {
        internal_(info);
        return Void();
    }

   private:
    Function internal_;
};

#define ASSERT_OK(r) ASSERT_TRUE(isOk(r))
#define EXPECT_OK(r) EXPECT_TRUE(isOk(r))
template <typename T>
::testing::AssertionResult isOk(const Return<T>& r) {
    return r.isOk() ? AssertionSuccess() : (AssertionFailure() << r.description());
}

#define ASSERT_ALL_OK(r) ASSERT_TRUE(isAllOk(r))
// Both isOk() and Result::SUCCESS
::testing::AssertionResult isAllOk(const Return<Result>& r) {
    if (!r.isOk()) {
        return AssertionFailure() << r.description();
    }
    if (static_cast<Result>(r) != Result::SUCCESS) {
        return AssertionFailure() << toString(static_cast<Result>(r));
    }
    return AssertionSuccess();
}

/**
 * Test whether callbacks work. Tested functions are IHealth::registerCallback,
 * unregisterCallback, and update.
 */
TEST_F(HealthHidlTest, Callbacks) {
    using namespace std::chrono_literals;

    std::mutex mutex;
    std::condition_variable cv;
    bool firstCallbackInvoked = false;
    bool secondCallbackInvoked = false;

    sp<Callback> firstCallback = new Callback([&](const auto&) {
        std::unique_lock<std::mutex> lk(cv);
        firstCallbackInvoked = true;
    });

    sp<Callback> secondCallback = new Callback([&](const auto&) {
        std::unique_lock<std::mutex> lk(cv);
        secondCallbackInvoked = true;
        cv.notify_all();
    });

    ASSERT_ALL_OK(health_->registerCallback(firstCallback));
    ASSERT_ALL_OK(health_->registerCallback(secondCallback));

    // assert that the first callback is invoked when update is called.
    {
        std::unique_lock<std::mutex> lk(cv);
        firstCallbackInvoked = false;
        secondCallbackInvoked = false;
    }

    ASSERT_ALL_OK(health_->update());

    {
        std::unique_lock<std::mutex> lk(cv);
        EXPECT_TRUE(cv.wait_for(lk, 1s, [&] {
            return firstCallbackInvoked && secondCallbackInvoked;
        })) << "Timeout.";
    }

    ASSERT_TRUE(firstCallbackInvoked);
    ASSERT_TRUE(secondCallbackInvoked);

    ASSERT_ALL_OK(health_->unregisterCallback(firstCallback));

    // assert that the second callback is still invoked even though the first is unregistered.
    {
        std::unique_lock<std::mutex> lk(cv);
        firstCallbackInvoked = false;
        secondCallbackInvoked = false;
    }

    ASSERT_ALL_OK(health_->update());

    {
        std::unique_lock<std::mutex> lk(cv);
        EXPECT_TRUE(cv.wait_for(lk, 1s, [&] { return secondCallbackInvoked; })) << "Timeout.";
    }

    ASSERT_FALSE(firstCallbackInvoked);
    ASSERT_TRUE(secondCallbackInvoked);

    ASSERT_ALL_OK(health_->unregisterCallback(secondCallback));
}

TEST_F(HealthHidlTest, UnregisterNonExistentCallback) {
    sp<Callback> callback = new Callback([](const auto&) {});
    auto ret = health_->unregisterCallback(callback);
    ASSERT_OK(ret);
    ASSERT_EQ(Result::NOT_FOUND, static_cast<Result>(ret));
}

/**
 * Pass the test if:
 *  - Property is not supported (res == NOT_SUPPORTED)
 *  - Result is success, and predicate is true
 * @param res the Result value.
 * @param valueStr the string representation for actual value (for error message)
 * @param pred a predicate that test whether the value is valid
 */
#define EXPECT_VALID_OR_UNSUPPORTED_PROP(res, valueStr, pred) \
    EXPECT_TRUE(isPropertyOk(res, valueStr, pred, #pred))

::testing::AssertionResult isPropertyOk(Result res, const std::string& valueStr, bool pred,
                                        const std::string& predStr) {
    if (res == Result::SUCCESS) {
        if (pred) {
            return AssertionSuccess();
        }
        return AssertionFailure() << "value doesn't match.\nActual: " << valueStr
                                  << "\nExpected: " << predStr;
    }
    if (res == Result::NOT_SUPPORTED) {
        return AssertionSuccess();
    }
    return AssertionFailure() << "Result is not SUCCESS or NOT_SUPPORTED: " << toString(res);
}

TEST_F(HealthHidlTest, Properties) {
    EXPECT_OK(health_->getChargeCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value > 0);
    }));
    EXPECT_OK(health_->getCurrentNow([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
    EXPECT_OK(health_->getCurrentAverage([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
    EXPECT_OK(health_->getCapacity([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), 0 <= value && value <= 100);
    }));
    EXPECT_OK(health_->getEnergyCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT64_MIN);
    }));
    EXPECT_OK(health_->getChargeStatus([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(
            result, toString(value),
            value == BatteryStatus::CHARGING || value == BatteryStatus::DISCHARGING ||
                value == BatteryStatus::NOT_CHARGING || value == BatteryStatus::FULL);
    }));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
