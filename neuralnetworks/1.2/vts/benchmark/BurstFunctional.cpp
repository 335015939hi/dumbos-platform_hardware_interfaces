/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <gtest/gtest.h>
#include <utils/StrongPointer.h>
#include <utils/Trace.h>
#include <memory>
#include <tuple>
#include "Callbacks.h"
#include "ExecutionBurstController.h"
#include "Utils.h"
#include "add_float.h"
#include "mobilenet_float.h"
#include "mobilenet_quantized.h"

// shorthand

namespace V1_0 = ::android::hardware::neuralnetworks::V1_0;
namespace V1_1 = ::android::hardware::neuralnetworks::V1_1;
namespace V1_2 = ::android::hardware::neuralnetworks::V1_2;

using V1_0::ErrorStatus;
using V1_0::Request;
using V1_0::toString;
using V1_1::ExecutionPreference;
using V1_2::IDevice;
using V1_2::IPreparedModel;
using V1_2::MeasureTiming;
using V1_2::Model;

using V1_2::implementation::ExecutionCallback;
using V1_2::implementation::PreparedModelCallback;

using ::android::sp;
using ::android::hardware::Return;
using ::android::nn::convertToV1_2;
using ::android::nn::ExecutionBurstController;

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
std::ostream& operator<<(std::ostream& os, ErrorStatus status) {
    return os << toString(status);
}
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

// Test Tests

using Check = std::function<bool(const Request&)>;

template <typename Fn>
Check make_function(Fn fn) {
    return {fn};
}

const auto kTestValues = ::testing::Values(
        std::make_tuple(convertToV1_2(add_float::createTestModel()), add_float::createTestRequest(),
                        make_function(add_float::check)),
        std::make_tuple(convertToV1_2(mobilenet_quantized::createTestModel()),
                        mobilenet_quantized::createTestRequest(),
                        make_function(mobilenet_quantized::check)),
        std::make_tuple(convertToV1_2(mobilenet_float::createTestModel()),
                        mobilenet_float::createTestRequest(),
                        make_function(mobilenet_float::check)));

class ModelExecution : public ::testing::TestWithParam<std::tuple<Model, Request, Check>> {
  public:
    void SetUp() override {
        ::testing::Test::SetUp();

        // parameters
        std::tie(model, request, check) = GetParam();

        // create memory keys for burst
        burstMemoryKeys.resize(request.pools.size());
        for (size_t i = 0; i < burstMemoryKeys.size(); ++i) {
            burstMemoryKeys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
        }

        // get device
        device = IDevice::getService("sample-all");
        ASSERT_NE(device, nullptr);

        // get capabilities to disable vlog
        device->getCapabilities_1_1([](auto, const auto&) {}).isOk();

        // build model
        const sp<PreparedModelCallback> callback = new PreparedModelCallback();
        ASSERT_NE(callback, nullptr);
        const Return<ErrorStatus> ret = device->prepareModel_1_2(
                model, ExecutionPreference::SUSTAINED_SPEED, {}, {}, {}, callback);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(ret, ErrorStatus::NONE);

        // get preparedModel
        callback->wait();
        const ErrorStatus status = callback->getStatus();
        ASSERT_EQ(status, ErrorStatus::NONE);
        preparedModel = IPreparedModel::castFrom(callback->getPreparedModel()).withDefault(nullptr);
        ASSERT_NE(preparedModel, nullptr);
    }

    void TearDown() override { ::testing::Test::TearDown(); }

  protected:
    Model model;
    Request request;
    std::vector<intptr_t> burstMemoryKeys;
    Check check;
    sp<IDevice> device;
    sp<IPreparedModel> preparedModel;
};

class ExecutionBurstBlocking : public ModelExecution {
  public:
    void SetUp() override {
        ModelExecution::SetUp();

        // variables
        burst = ExecutionBurstController::create(preparedModel, true);
        ASSERT_NE(burst, nullptr);
    }

    void TearDown() override { ModelExecution::TearDown(); }

  protected:
    std::shared_ptr<ExecutionBurstController> burst;
};

class ExecutionBurstSpinning : public ModelExecution {
  public:
    void SetUp() override {
        ModelExecution::SetUp();

        // variables
        burst = ExecutionBurstController::create(preparedModel, false);
        ASSERT_NE(burst, nullptr);
    }

    void TearDown() override { ModelExecution::TearDown(); }

  protected:
    std::shared_ptr<ExecutionBurstController> burst;
};

// Benchmarks

TEST_P(ModelExecution, BM_Time) {
    sp<ExecutionCallback> callback = new ExecutionCallback();
    ErrorStatus status = ErrorStatus::GENERAL_FAILURE;
    Return<ErrorStatus> ret = ErrorStatus::GENERAL_FAILURE;

    {
        // systrace to see IPC overhead
        ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ModelExecution::BM_Time");

        // start execution
        ret = preparedModel->execute_1_2(request, MeasureTiming::NO, callback);

        // wait for response
        callback->wait();
        status = callback->getStatus();
    }

    // error handling
    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(status, ErrorStatus::NONE);

    // verify output
    EXPECT_TRUE(check(request));
}

TEST_P(ExecutionBurstBlocking, BM_Time) {
    ErrorStatus status;
    std::vector<OutputShape> outputShapes;

    {
        // systrace to see IPC overhead
        ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ExecutionBurst::BM_Time");

        // perform execution
        std::tie(status, outputShapes, std::ignore) =
                burst->compute(request, MeasureTiming::NO, burstMemoryKeys);
    }

    // error checking
    ASSERT_EQ(status, ErrorStatus::NONE);

    // verify output
    EXPECT_TRUE(check(request));
}

TEST_P(ExecutionBurstSpinning, BM_Time) {
    ErrorStatus status;
    std::vector<OutputShape> outputShapes;

    {
        // systrace to see IPC overhead
        ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ExecutionBurst::BM_Time");

        // perform execution
        std::tie(status, outputShapes, std::ignore) =
                burst->compute(request, MeasureTiming::NO, burstMemoryKeys);
    }

    // error checking
    ASSERT_EQ(status, ErrorStatus::NONE);

    // verify output
    EXPECT_TRUE(check(request));
}

INSTANTIATE_TEST_CASE_P(Flavor, ModelExecution, kTestValues);
INSTANTIATE_TEST_CASE_P(Flavor, ExecutionBurstBlocking, kTestValues);
INSTANTIATE_TEST_CASE_P(Flavor, ExecutionBurstSpinning, kTestValues);
