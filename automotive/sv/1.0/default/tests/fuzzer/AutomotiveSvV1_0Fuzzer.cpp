/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "AutomotiveSvV1_0Fuzzer.h"
#include <SurroundViewStream.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidlmemory/mapping.h>

namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer {

using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hidl::allocator::V1_0::IAllocator;

constexpr uint32_t kMinConfigDimension = 1;
constexpr uint32_t kMaxConfigDimension = 4096;
constexpr uint32_t kVertexByteSize = (3 * sizeof(float)) + 4;
constexpr uint32_t kIdByteSize = 2;
constexpr size_t kMaxCharacters = 30;
constexpr size_t kMaxVertices = 10;
constexpr size_t kMaxCameraPoints = 10;
constexpr size_t kMaxViews = 10;
constexpr size_t kMaxOverlays = 10;

void SurroundViewFuzzer::invoke2dSessionAPI() {
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
            [&surroundView2dSession](const sp<ISurroundView2dSession>& session, SvResult result) {
                if (result == SvResult::OK) {
                    surroundView2dSession = session;
                }
            });

    if (!surroundView2dSession) {
        return;
    }

    sp<SurroundViewStream> handler = sp<SurroundViewStream>::make(surroundView2dSession);

    surroundView2dSession->startStream(handler);
    surroundView2dSession->get2dMappingInfo([]([[maybe_unused]] Sv2dMappingInfo info) {});

    Sv2dConfig config;
    config.width = mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(kMinConfigDimension,
                                                                         kMaxConfigDimension);
    config.blending = mFuzzedDataProvider->ConsumeBool() ? (SvQuality::HIGH) : (SvQuality::LOW);
    surroundView2dSession->set2dConfig(config);

    Sv2dConfig retConfig;
    surroundView2dSession->get2dConfig([&retConfig](Sv2dConfig config) {
        retConfig.width = config.width;
        retConfig.blending = config.blending;
    });

    hidl_vec<Point2dInt> points2dCamera;
    const size_t camPoints =
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(1, kMaxCameraPoints);
    points2dCamera.resize(camPoints);
    for (size_t i = 0; i < camPoints; ++i) {
        points2dCamera[i].x = mFuzzedDataProvider->ConsumeFloatingPoint<float>();
        points2dCamera[i].y = mFuzzedDataProvider->ConsumeFloatingPoint<float>();
    }

    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds(
            [&cameraIds](const hidl_vec<hidl_string>& camIds) { cameraIds = camIds; });
    hidl_string cameraId;
    if (cameraIds.size() > 0) {
        const size_t cameraIndex =
                mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, cameraIds.size() - 1);
        cameraId = cameraIds[cameraIndex];
    } else {
        cameraId = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);
    }
    surroundView2dSession->projectCameraPoints(
            points2dCamera, cameraId,
            []([[maybe_unused]] const hidl_vec<Point2dFloat>& outPoints) {});

    SvFramesDesc frames;
    surroundView2dSession->doneWithFrames(frames);

    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

void SurroundViewFuzzer::invoke3dSessionAPI() {
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
            [&surroundView3dSession](const sp<ISurroundView3dSession>& session, SvResult result) {
                if (result == SvResult::OK) {
                    surroundView3dSession = session;
                }
            });

    if (!surroundView3dSession) {
        return;
    }

    sp<SurroundViewStream> handler = sp<SurroundViewStream>::make(surroundView3dSession);

    const size_t numViews = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(1, kMaxViews);
    std::vector<View3d> views(numViews);
    for (size_t i = 0; i < numViews; ++i) {
        views[i].viewId = i;
    }
    surroundView3dSession->setViews(views);

    surroundView3dSession->startStream(handler);

    Sv3dConfig config;
    config.width = mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(kMinConfigDimension,
                                                                         kMaxConfigDimension);
    config.height = mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(kMinConfigDimension,
                                                                          kMaxConfigDimension);
    config.carDetails = mFuzzedDataProvider->ConsumeBool() ? (SvQuality::HIGH) : (SvQuality::LOW);
    surroundView3dSession->set3dConfig(config);

    Sv3dConfig retConfig;
    surroundView3dSession->get3dConfig([&retConfig](Sv3dConfig config) {
        retConfig.width = config.width;
        retConfig.height = config.height;
        retConfig.carDetails = config.carDetails;
    });

    Point2dInt cameraPoint;
    cameraPoint.x = mFuzzedDataProvider->ConsumeFloatingPoint<float>();
    cameraPoint.y = mFuzzedDataProvider->ConsumeFloatingPoint<float>();
    std::vector<Point2dInt> cameraPoints = {cameraPoint};
    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds(
            [&cameraIds](const hidl_vec<hidl_string>& camIds) { cameraIds = camIds; });
    hidl_string cameraId;
    if (cameraIds.size() > 0) {
        const size_t cameraIndex =
                mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, cameraIds.size() - 1);
        cameraId = cameraIds[cameraIndex];
    } else {
        cameraId = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);
    }
    std::vector<Point3dFloat> points3d;
    surroundView3dSession->projectCameraPointsTo3dSurface(
            cameraPoints, cameraId,
            [&points3d]([[maybe_unused]] const hidl_vec<Point3dFloat>& points3dproj) {
                points3d = points3dproj;
            });

    const uint8_t action = mFuzzedDataProvider->ConsumeIntegralInRange<uint8_t>(0, 2);
    initSampleOverlaysData();

    switch (action) {
        case 0: {
            // success case
            surroundView3dSession->updateOverlays(mOverlaysdata);
            break;
        }
        case 1: {
            // Fail with ID mismatch
            // Set id of second overlay in shared memory to 2 (expected is 1).
            auto& overlaysDescVector = mOverlaysdata.overlaysMemoryDesc;
            auto& pIMemory = mMemory;
            int32_t indexPosition =
                    mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(0, mNumOverlays - 1);
            int32_t mismatchedValueIndex =
                    mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(0, mNumOverlays - 1);
            setIndexOfOverlaysMemory(overlaysDescVector, pIMemory, indexPosition,
                                     overlaysDescVector[mismatchedValueIndex].id);

            surroundView3dSession->updateOverlays(mOverlaysdata);
            break;
        }
        case 2: {
            // Fail with NULL memory
            // Set shared memory to null.
            mOverlaysdata.overlaysMemory = hidl_memory();
            surroundView3dSession->updateOverlays(mOverlaysdata);
            break;
        }
        default:
            break;
    }

    SvFramesDesc frames;
    surroundView3dSession->doneWithFrames(frames);

    surroundView3dSession->stopStream();
    mSurroundViewService->stop3dSession(surroundView3dSession);
}

void SurroundViewFuzzer::process(const uint8_t* data, size_t size) {
    mFuzzedDataProvider = new FuzzedDataProvider(data, size);
    invoke2dSessionAPI();
    invoke3dSessionAPI();
}

std::pair<hidl_memory, sp<IMemory>> SurroundViewFuzzer::getMappedSharedMemory(int32_t bytesSize) {
    const auto nullResult = std::make_pair(hidl_memory(), nullptr);

    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator.get() == nullptr) {
        return nullResult;
    }

    // Allocate shared memory.
    hidl_memory hidlMemory;
    bool allocateSuccess = false;
    Return<void> result =
            ashmemAllocator->allocate(bytesSize, [&](bool success, const hidl_memory& hidlMem) {
                if (!success) {
                    return;
                }
                allocateSuccess = success;
                hidlMemory = hidlMem;
            });

    // Check result of allocated memory.
    if (!result.isOk() || !allocateSuccess) {
        return nullResult;
    }

    // Map shared memory.
    sp<IMemory> pIMemory = mapMemory(hidlMemory);
    if (pIMemory.get() == nullptr) {
        return nullResult;
    }

    return std::make_pair(hidlMemory, pIMemory);
}

void SurroundViewFuzzer::setIndexOfOverlaysMemory(
        const std::vector<OverlayMemoryDesc>& overlaysMemDesc, sp<IMemory> pIMemory,
        int32_t indexPosition, uint16_t indexValue) {
    // Count the number of vertices until the index.
    int32_t totalVerticesCount = 0;
    for (int32_t i = 0; i < indexPosition; ++i) {
        totalVerticesCount += overlaysMemDesc[i].verticesCount;
    }

    const int32_t indexBytePosition =
            (indexPosition * kIdByteSize) + (kVertexByteSize * totalVerticesCount);

    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pSharedMemoryData += indexBytePosition;
    uint16_t* pIndex16bit = (uint16_t*)pSharedMemoryData;

    // Modify shared memory.
    pIMemory->update();
    *pIndex16bit = indexValue;
    pIMemory->commit();
}

void SurroundViewFuzzer::initSampleOverlaysData() {
    const size_t mNumOverlays =
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinOverlays, kMaxOverlays);
    mOverlaysdata.overlaysMemoryDesc.resize(mNumOverlays);

    int32_t sharedMemBytesSize = 0;
    std::vector<OverlayMemoryDesc> overlaysDescVector = {};
    OverlayMemoryDesc overlayMemDesc[mNumOverlays];
    for (size_t i = 0; i < mNumOverlays; ++i) {
        overlayMemDesc[i].id = i;
        overlayMemDesc[i].verticesCount =
                mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(1, kMaxVertices);
        overlayMemDesc[i].overlayPrimitive = mFuzzedDataProvider->ConsumeBool()
                                                     ? (OverlayPrimitive::TRIANGLES)
                                                     : (OverlayPrimitive::TRIANGLES_STRIP);
        mOverlaysdata.overlaysMemoryDesc[i] = overlayMemDesc[i];

        sharedMemBytesSize += kIdByteSize + kVertexByteSize * overlayMemDesc[i].verticesCount;
        overlaysDescVector.push_back(overlayMemDesc[i]);
    }

    std::pair<hidl_memory, sp<IMemory>> sharedMem = getMappedSharedMemory(sharedMemBytesSize);
    sp<IMemory> pIMemory = std::get<1>(sharedMem);
    if (pIMemory.get() == nullptr) {
        mOverlaysdata = OverlaysData();
        mMemory = nullptr;
        return;
    }

    // Get pointer to shared memory data and set all bytes to 0.
    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pIMemory->update();
    memset(pSharedMemoryData, 0, sharedMemBytesSize);
    pIMemory->commit();

    // Set indexes in shared memory.
    for (size_t i = 0; i < mNumOverlays; ++i) {
        setIndexOfOverlaysMemory(overlaysDescVector, pIMemory, i, overlayMemDesc[i].id);
    }

    mOverlaysdata.overlaysMemoryDesc = overlaysDescVector;
    mOverlaysdata.overlaysMemory = std::get<0>(sharedMem);
    mMemory = pIMemory;
}

bool SurroundViewFuzzer::init() {
    mSurroundViewService = sp<SurroundViewService>::make();
    if (!mSurroundViewService) {
        return false;
    }
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) {
        return 0;
    }
    SurroundViewFuzzer surroundViewFuzzer;
    if (surroundViewFuzzer.init()) {
        surroundViewFuzzer.process(data, size);
    }
    return 0;
}
}  // namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer
