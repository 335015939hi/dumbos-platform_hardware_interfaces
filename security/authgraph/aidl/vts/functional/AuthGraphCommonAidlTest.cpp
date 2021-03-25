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

#define LOG_TAG "authgraph_common_test"
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/authgraph/IAuthGraphCommon.h>
#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <vector>

namespace aidl::android::hardware::security::authgraph::test {
using ::aidl::android::hardware::security::keymint::ErrorCode;
using ::std::shared_ptr;
using ::std::string;
using ::std::vector;
using Status = ::ndk::ScopedAStatus;

class AuthGraphCommonAidlTest : public ::testing::TestWithParam<string> {
  public:
    struct InitChannelResponse {
        ErrorCode error;
        InitChannelResult init_channel_result;
    };

    InitChannelResponse initChannel(vector<uint8_t> signingKeySetupInfo) {
        LOG(INFO) << "In initChannel VTS test method.";
        InitChannelResponse response;
        response.error = GetReturnErrorCode(
                authGraphCommon_->initChannel(signingKeySetupInfo, &response.init_channel_result));
        return response;
    }

    ErrorCode GetReturnErrorCode(const Status& result) {
        if (result.isOk()) return ErrorCode::OK;

        if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
            return static_cast<ErrorCode>(result.getServiceSpecificError());
        }

        return ErrorCode::UNKNOWN_ERROR;
    }
    void InitializeAuthGraphCommon(std::shared_ptr<IAuthGraphCommon> authGraphCommon) {
        ASSERT_NE(authGraphCommon, nullptr);
        authGraphCommon_ = authGraphCommon;
    }

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IAuthGraphCommon::descriptor);
        return params;
    }

    void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            InitializeAuthGraphCommon(IAuthGraphCommon::fromBinder(binder));
        } else {
            InitializeAuthGraphCommon(nullptr);
        }
    }

    void TearDown() override {}

  private:
    std::shared_ptr<IAuthGraphCommon> authGraphCommon_;
};

// The simplest test to make sure that the reference implementation is invoked.
TEST_P(AuthGraphCommonAidlTest, TestInitChannel) {
    // Pass an empty vector for testing
    auto response = initChannel(std::vector<uint8_t>());
    EXPECT_EQ(ErrorCode::OPERATION_NOT_SUPPORTED, response.error);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, AuthGraphCommonAidlTest,
                         testing::ValuesIn(AuthGraphCommonAidlTest::build_params()),
                         ::android::PrintInstanceNameToString);
}  // namespace aidl::android::hardware::security::authgraph::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
