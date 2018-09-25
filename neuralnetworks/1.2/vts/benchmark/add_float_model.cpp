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

// includes
#include "add_float.h"

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <vector>

// namespace
namespace add_float {
namespace {

using ::android::sp;
using ::android::hardware::hidl_memory;
using ::android::hardware::Return;
using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::Model;
using ::android::hardware::neuralnetworks::V1_0::Operand;
using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::OperandType;
using ::android::hardware::neuralnetworks::V1_0::Operation;
using ::android::hardware::neuralnetworks::V1_0::OperationType;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

}  // anonymous namespace

// Create the model
Model createTestModel() {
    const std::vector<Operand> operands = {
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {2},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_INPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {2},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_INPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 0, .length = 4},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {2},
            .numberOfConsumers = 0,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_OUTPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        }};

    const std::vector<Operation> operations = {{
        .type = OperationType::ADD,
        .inputs = {0, 1, 2},
        .outputs = {3},
    }};

    const std::vector<uint32_t> inputIndexes = {0, 1};
    const std::vector<uint32_t> outputIndexes = {3};
    std::vector<uint8_t> operandValues = {0, 0, 0, 0};
    const std::vector<hidl_memory> pools = {};

    return {
        .operands = operands,
        .operations = operations,
        .inputIndexes = inputIndexes,
        .outputIndexes = outputIndexes,
        .operandValues = operandValues,
        .pools = pools,
    };
}

}  // namespace add_float
