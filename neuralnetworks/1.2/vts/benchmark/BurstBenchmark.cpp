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

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <benchmark/benchmark.h>
#include <utils/StrongPointer.h>
#include <utils/Trace.h>
#include <memory>
#include <tuple>
#include "Callbacks.h"
#include "FmqChannel.h"
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
using V1_2::Model;

using V1_2::implementation::ExecutionCallback;
using V1_2::implementation::PreparedModelCallback;

using ::android::sp;
using ::android::hardware::Return;
using ::android::nn::convertToV1_2;
using ::android::nn::createExecutionBurstController;
using ::android::nn::IExecutionBurstController;

// Test Fixtures

const std::vector<std::pair<Model, Request>> kTestValues = {
    std::make_pair(convertToV1_2(add_float::createTestModel()), add_float::createTestRequest()),
    std::make_pair(convertToV1_2(mobilenet_quantized::createTestModel()),
                   mobilenet_quantized::createTestRequest()),
    std::make_pair(convertToV1_2(mobilenet_float::createTestModel()),
                   mobilenet_float::createTestRequest())};

class ModelExecution : public ::benchmark::Fixture {
   public:
    void SetUp(::benchmark::State& state) override {
        ::benchmark::Fixture::SetUp(state);

        // parameters
        const int argIndex = state.range(0);
        std::tie(model, request) = kTestValues[argIndex];

        // get device
        device = IDevice::getService("sample-all");
        if (device == nullptr) {
            state.SkipWithError("Failed to retrieve service.");
        }

        // get capabilities to disable vlog
        device->getCapabilities_1_1([](auto, const auto&) {}).isOk();

        // build model
        const sp<PreparedModelCallback> callback = new PreparedModelCallback();
        if (callback == nullptr) {
            state.SkipWithError("Failed to allocate callback object.");
        }
        const Return<ErrorStatus> ret =
            device->prepareModel_1_2(model, ExecutionPreference::SUSTAINED_SPEED, callback);
        if (!ret.isOk()) {
            state.SkipWithError("Hidl transport error for IDevice::prepare.");
        }
        if (ret != ErrorStatus::NONE) {
            state.SkipWithError("IDevice::prepare returned launch error.");
        }

        // get preparedModel
        callback->wait();
        const ErrorStatus status = callback->getStatus();
        if (status != ErrorStatus::NONE) {
            state.SkipWithError("IDevice::prepare returned async error.");
        }
        preparedModel = IPreparedModel::castFrom(callback->getPreparedModel()).withDefault(nullptr);
        if (preparedModel == nullptr) {
            state.SkipWithError("Failed to retrieve preparedModel.");
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        ::benchmark::Fixture::TearDown(state);
    }

   protected:
    Model model;
    Request request;
    sp<IDevice> device;
    sp<IPreparedModel> preparedModel;
};

class ExecutionBurstBlocking : public ModelExecution {
   public:
    void SetUp(::benchmark::State& state) override {
        ModelExecution::SetUp(state);

        // variables
        burst = createExecutionBurstController(preparedModel, request, true);
        if (burst == nullptr) {
            state.SkipWithError("createExecutionBurstController error.");
        }
    }

    void TearDown(const ::benchmark::State& state) override { ModelExecution::TearDown(state); }

   protected:
    std::unique_ptr<IExecutionBurstController> burst;
};

class ExecutionBurstSpinning : public ModelExecution {
   public:
    void SetUp(::benchmark::State& state) override {
        ModelExecution::SetUp(state);

        // variables
        burst = createExecutionBurstController(preparedModel, request, false);
        if (burst == nullptr) {
            state.SkipWithError("createExecutionBurstController error.");
        }
    }

    void TearDown(const ::benchmark::State& state) override { ModelExecution::TearDown(state); }

   protected:
    std::unique_ptr<IExecutionBurstController> burst;
};

// Benchmarks

BENCHMARK_DEFINE_F(ModelExecution, BM_Time)(benchmark::State& state) {
    for (auto _ : state) {
        sp<ExecutionCallback> callback = new ExecutionCallback();
        ErrorStatus status = ErrorStatus::GENERAL_FAILURE;
        Return<ErrorStatus> ret = ErrorStatus::GENERAL_FAILURE;

        {
            // systrace to see IPC overhead
            ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ModelExecution::BM_Time");

            // start execution
            ret = preparedModel->execute(request, callback);

            // wait for response
            callback->wait();
            status = callback->getStatus();
        }

        // error handling
        if (!ret.isOk()) {
            state.SkipWithError("Hidl transport error for ModelExecution.");
        }
        if (ret.isOk() && status != ErrorStatus::NONE) {
            state.SkipWithError(("ModelExecution returned an error: " + toString(status)).c_str());
        }
    }
}

BENCHMARK_DEFINE_F(ExecutionBurstBlocking, BM_Time)(benchmark::State& state) {
    for (auto _ : state) {
        // start execution
        const bool launched = burst->launch(ErrorStatus::NONE);
        if (!launched) {
            state.SkipWithError("Failed to launch burst execution.");
        }

        // systrace to see IPC overhead
        ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ExecutionBurst::BM_Time");

        // wait for response
        const ErrorStatus status = burst->getResult();

        // error checking
        if (status != ErrorStatus::NONE) {
            state.SkipWithError(("ExecutionBurst returned an error: " + toString(status)).c_str());
        }
    }
}

BENCHMARK_DEFINE_F(ExecutionBurstSpinning, BM_Time)(benchmark::State& state) {
    for (auto _ : state) {
        // start execution
        const bool launched = burst->launch(ErrorStatus::NONE);
        if (!launched) {
            state.SkipWithError("Failed to launch burst execution.");
        }

        // systrace to see IPC overhead
        ::android::ScopedTrace trace(ATRACE_TAG_NNAPI, "ExecutionBurst::BM_Time");

        // wait for response
        const ErrorStatus status = burst->getResult();

        // error checking
        if (status != ErrorStatus::NONE) {
            state.SkipWithError(("ExecutionBurst returned an error: " + toString(status)).c_str());
        }
    }
}

BENCHMARK_REGISTER_F(ModelExecution, BM_Time)->Range(0, kTestValues.size() - 1);
BENCHMARK_REGISTER_F(ExecutionBurstBlocking, BM_Time)->Range(0, kTestValues.size() - 1);
BENCHMARK_REGISTER_F(ExecutionBurstSpinning, BM_Time)->Range(0, kTestValues.size() - 1);

// Main

BENCHMARK_MAIN();
