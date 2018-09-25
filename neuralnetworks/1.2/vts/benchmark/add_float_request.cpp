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

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <cmath>
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

// allocate shared memory utility
hidl_memory allocateSharedMemory(int64_t size) {
    static sp<IAllocator> allocator = IAllocator::getService("ashmem");
    hidl_memory memory;
    if (allocator != nullptr) {
        allocator->allocate(size, [&memory](bool, const hidl_memory& mem) { memory = mem; });
    }
    return memory;
}

const float input1[] = {1.0f, 2.0f};
const float input2[] = {3.0f, 4.0f};
const float goldenOutput[] = {4.0f, 6.0f};

bool closeEnough(float actual, float golden) {
    constexpr const float fpAtol = 1e-5f;
    constexpr const float fpRtol = 5.0f * 1.1920928955078125e-7f;
    const float fpRange = fpAtol + fpRtol * std::abs(golden);

    const float err = std::abs(actual - golden);
    if (!(err < fpRange)) {
        LOG(ERROR) << "error is " << err << " vs tolerance of " << fpRange;
    }

    return err < fpRange;
}

}  // anonymous namespace

Request createTestRequest() {
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    std::vector<hidl_memory> pools = {allocateSharedMemory(sizeof(input1) + sizeof(input2)),
                                      allocateSharedMemory(sizeof(goldenOutput))};

    RequestArgument input_info1 = {
        .hasNoValue = false,
        .location = {.poolIndex = INPUT, .offset = 0, .length = sizeof(input1)},
        .dimensions = {}};
    RequestArgument input_info2 = {
        .hasNoValue = false,
        .location = {.poolIndex = INPUT, .offset = sizeof(input1), .length = sizeof(input2)},
        .dimensions = {}};
    RequestArgument output_info = {
        .hasNoValue = false,
        .location = {.poolIndex = OUTPUT, .offset = 0, .length = sizeof(goldenOutput)},
        .dimensions = {}};

    // copy input data
    const sp<IMemory> inputMemory = mapMemory(pools[INPUT]);
    if (inputMemory == nullptr) {
        LOG(ERROR) << "unable to map inputMemory";
        return {};
    }
    float* inputPtr = static_cast<float*>(static_cast<void*>(inputMemory->getPointer()));
    if (inputPtr == nullptr) {
        LOG(ERROR) << "unable to get pointer inputPtr";
        return {};
    }

    inputMemory->update();
    std::copy(std::begin(input1), std::end(input1), inputPtr);
    std::copy(std::begin(input2), std::end(input2), inputPtr + sizeof(input1) / sizeof(*input1));
    inputMemory->commit();

    // clear output data
    const sp<IMemory> outputMemory = mapMemory(pools[OUTPUT]);
    if (outputMemory == nullptr) {
        LOG(ERROR) << "unable to map outputMemory";
        return {};
    }
    float* outputPtr = static_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
    if (outputPtr == nullptr) {
        LOG(ERROR) << "unable to get pointer outputPtr";
        return {};
    }

    outputMemory->update();
    std::fill_n(outputPtr, sizeof(goldenOutput), float{0});
    outputMemory->commit();

    // return finished Request
    return {.inputs = {input_info1, input_info2}, .outputs = {output_info}, .pools = pools};
}

bool check(const Request& request) {
    const uint32_t OUTPUT = 1;
    const uint32_t length = sizeof(goldenOutput) / sizeof(goldenOutput[0]);

    // unpack output
    const sp<IMemory> outputMemory = mapMemory(request.pools[OUTPUT]);
    if (outputMemory == nullptr) {
        LOG(ERROR) << "unable to map outputMemory";
        return {};
    }
    float* outputPtr = static_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
    if (outputPtr == nullptr) {
        LOG(ERROR) << "unable to get pointer outputPtr";
        return {};
    }

    // check
    outputMemory->read();
    const bool match = std::equal(outputPtr, outputPtr + length, std::begin(goldenOutput),
                                  std::end(goldenOutput), closeEnough);
    outputMemory->commit();

    // return
    return match;
}

}  // namespace add_float
