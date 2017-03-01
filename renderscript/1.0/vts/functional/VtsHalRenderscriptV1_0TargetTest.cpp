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

#define LOG_TAG "renderscript_hidl_hal_test"
#include "bitcode.rs"
#include <android-base/logging.h>

#include <android/hardware/renderscript/1.0/IContext.h>
#include <android/hardware/renderscript/1.0/IDevice.h>
#include <android/hardware/renderscript/1.0/types.h>

#include <VtsHalHidlTargetTestBase.h>
#include <gtest/gtest.h>

using ::android::hardware::renderscript::V1_0::Allocation;
using ::android::hardware::renderscript::V1_0::AllocationAdapter;
using ::android::hardware::renderscript::V1_0::AllocationCubemapFace;
using ::android::hardware::renderscript::V1_0::AllocationMipmapControl;
using ::android::hardware::renderscript::V1_0::AllocationUsageType;
using ::android::hardware::renderscript::V1_0::IContext;
using ::android::hardware::renderscript::V1_0::IDevice;
using ::android::hardware::renderscript::V1_0::ContextType;
using ::android::hardware::renderscript::V1_0::DataType;
using ::android::hardware::renderscript::V1_0::DataKind;
using ::android::hardware::renderscript::V1_0::Element;
using ::android::hardware::renderscript::V1_0::MessageToClientType;
using ::android::hardware::renderscript::V1_0::NativeWindow;
using ::android::hardware::renderscript::V1_0::ObjectBase;
using ::android::hardware::renderscript::V1_0::OpaqueHandle;
using ::android::hardware::renderscript::V1_0::Ptr;
using ::android::hardware::renderscript::V1_0::Sampler;
using ::android::hardware::renderscript::V1_0::SamplerValue;
using ::android::hardware::renderscript::V1_0::Script;
using ::android::hardware::renderscript::V1_0::ScriptFieldID;
using ::android::hardware::renderscript::V1_0::ScriptGroup;
using ::android::hardware::renderscript::V1_0::ScriptGroup2;
using ::android::hardware::renderscript::V1_0::ScriptIntrinsicID;
using ::android::hardware::renderscript::V1_0::ScriptInvokeID;
using ::android::hardware::renderscript::V1_0::ScriptKernelID;
using ::android::hardware::renderscript::V1_0::Size;
using ::android::hardware::renderscript::V1_0::ThreadPriorities;
using ::android::hardware::renderscript::V1_0::Type;
using ::android::hardware::renderscript::V1_0::YuvFormat;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

// The main test class for RENDERSCRIPT HIDL HAL.
class RenderscriptHidlTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        //device = IDevice::getService();
        device = ::testing::VtsHalHidlTargetTestBase::getService<IDevice>();
        ASSERT_NE(nullptr, device.get());

        uint32_t version = 0;
        uint32_t flags = 0;
        context = device->contextCreate(version, ContextType::NORMAL, flags);
        ASSERT_NE(nullptr, context.get());
    }

    virtual void TearDown() override {
        context->contextDestroy();
    }

    sp<IContext>   context;

private:
    sp<IDevice>    device;
};

// A class for test environment setup (kept since this file is a template).
class HidlEnvironment : public ::testing::Environment {
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

/*
 * ContextCreateAndDestroy:
 * Creates a RenderScript context and immediately destroys the context.
 * Since create and destroy calls are a part of SetUp() and TearDown(),
 * the function definition is intentionally kept empty
 */
TEST_F(RenderscriptHidlTest, ContextCreateAndDestroy) {}

/*
 *
 */
TEST_F(RenderscriptHidlTest, ElementCreate) {
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    EXPECT_NE(Element(0), element);
}

TEST_F(RenderscriptHidlTest, ElementTypeAllocationCreate) {
    // Element create test
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    EXPECT_NE(Element(0), element);

    // Type create test
    Type type = context->typeCreate(element, 1, 0, 0, false, false, YuvFormat::YUV_NONE);
    EXPECT_NE(Type(0), type);

    // Allocation create test
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)((uint32_t)AllocationUsageType::ALL
                                                           & ~(uint32_t)AllocationUsageType::OEM),
                                                           (Ptr)nullptr);
    EXPECT_NE(Allocation(0), allocation);

    // Allocation type test
    Type type2 = context->allocationGetType(allocation);
    EXPECT_EQ(type, type2);
}

TEST_F(RenderscriptHidlTest, Simple1DCopyTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x float1
    Type type = context->typeCreate(element, 128, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    std::vector<float> dataIn(128), dataOut(128);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    context->allocation1DWrite(allocation, 0, 0, (Size)dataIn.size(), _data);
    context->allocation1DRead(allocation, 0, 0, (uint32_t)dataOut.size(), (Ptr)dataOut.data(),
                              (Size)dataOut.size()*sizeof(float));
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](float x){ static int val = 0; return x == (float)val++; });
    EXPECT_EQ(true, same);
}

TEST_F(RenderscriptHidlTest, Simple2DCopyTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x 128 x float1
    Type type = context->typeCreate(element, 128, 128, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x 128 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    std::vector<float> dataIn(128*128), dataOut(128*128);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    context->allocation2DWrite(allocation, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 128, 128,
                               _data, 0);
    context->allocation2DRead(allocation, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 128, 128,
                              (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(float), 0);
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](float x){ static int val = 0; return x == (float)val++; });
    EXPECT_EQ(true, same);
}

TEST_F(RenderscriptHidlTest, Simple3DCopyTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 32 x 32 x 32 x float1
    Type type = context->typeCreate(element, 32, 32, 32, false, false, YuvFormat::YUV_NONE);
    // 32 x 32 x 32 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    std::vector<float> dataIn(32*32*32), dataOut(32*32*32);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    context->allocation3DWrite(allocation, 0, 0, 0, 0, 32, 32, 32, _data, 0);
    context->allocation3DRead(allocation, 0, 0, 0, 0, 32, 32, 32, (Ptr)dataOut.data(),
                              (Size)dataOut.size()*sizeof(float), 0);
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](float x){ static int val = 0; return x == (float)val++; });
    EXPECT_EQ(true, same);
}

TEST_F(RenderscriptHidlTest, SimpleBitmapTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 512 x 512 x float1
    Type type = context->typeCreate(element, 512, 512, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(512*512), dataOut1(512*512), dataOut2(512*512);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 512 x 512 x float1
    Allocation allocation = context->allocationCreateFromBitmap(type,
                                                                AllocationMipmapControl::NONE,
                                                                _data,
                                                                (int)AllocationUsageType::SCRIPT);
    EXPECT_NE(allocation, Allocation(0));

    context->allocationCopyToBitmap(allocation, (Ptr)dataOut1.data(),
                                    (Size)dataOut1.size()*sizeof(float));
    bool same1 = std::all_of(dataOut1.begin(), dataOut1.end(),
                             [](float x){ static int val = 0; return x == (float)val++; });
    EXPECT_EQ(true, same1);

    context->allocationRead(allocation, (Ptr)dataOut2.data(), (Size)dataOut2.size()*sizeof(float));
    bool same2 = std::all_of(dataOut2.begin(), dataOut2.end(),
                             [](float x){ static int val = 0; return x == (float)val++; });
    EXPECT_EQ(true, same2);
}

TEST_F(RenderscriptHidlTest, AllocationCopy2DRangeTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 512 x 512 x float1
    Type typeSrc = context->typeCreate(element, 512, 512, 0, false, false, YuvFormat::YUV_NONE);
    // 256 x 256 x float1
    Type typeDst = context->typeCreate(element, 256, 256, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(512*512), dataOut(256*256), expected(256*256);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 512 x 512 x float1
    Allocation allocSrc = context->allocationCreateFromBitmap(typeSrc,
                                                              AllocationMipmapControl::NONE, _data,
                                                              (int)AllocationUsageType::SCRIPT);
    // 256 x 256 x float1
    Allocation allocDst = context->allocationCreateTyped(typeDst, AllocationMipmapControl::NONE,
                                                         (int)AllocationUsageType::SCRIPT,
                                                         (Ptr)nullptr);
    context->allocationCopy2DRange(allocDst, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 256, 256,
                                   allocSrc, 128, 128, 0, AllocationCubemapFace::POSITIVE_X);
    context->allocationRead(allocDst, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(float));
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 256; ++j) {
            expected[i*256 + j] = dataIn[(i+128)*512 + (j+128)];
        }
    }
    EXPECT_EQ(true, dataOut == expected);
}

TEST_F(RenderscriptHidlTest, AllocationCopy3DRangeTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x 128 x 128 x float1
    Type typeSrc = context->typeCreate(element, 128, 128, 128, false, false, YuvFormat::YUV_NONE);
    // 64 x 64 x 64 x float1
    Type typeDst = context->typeCreate(element, 64, 64, 64, false, false, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(128*128*128), dataOut(64*64*64), expected(64*64*64);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 512 x 512 x float1
    Allocation allocSrc = context->allocationCreateTyped(typeSrc, AllocationMipmapControl::NONE,
                                                         (int)AllocationUsageType::SCRIPT,
                                                         (Ptr)nullptr);
    // 256 x 256 x float1
    Allocation allocDst = context->allocationCreateTyped(typeDst, AllocationMipmapControl::NONE,
                                                         (int)AllocationUsageType::SCRIPT,
                                                         (Ptr)nullptr);
    context->allocation3DWrite(allocSrc, 0, 0, 0, 0, 128, 128, 128, _data, 128*sizeof(float));
    context->allocationCopy3DRange(allocDst, 0, 0, 0, 0, 64, 64, 64, allocSrc, 32, 32, 32, 0);
    context->allocationRead(allocDst, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(float));
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            for (int k = 0; k < 64; ++k) {
                expected[i*64*64 + j*64 + k] = dataIn[(i+32)*128*128 + (j+32)*128 + (k+32)];
            }
        }
    }
    EXPECT_EQ(true, dataOut == expected);
}

TEST_F(RenderscriptHidlTest, SimpleAdapterTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 512 x 512 x float1
    Type type = context->typeCreate(element, 512, 512, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(512*512), dataOut(256*256), expected;
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 512 x 512 x float1
    Allocation allocation = context->allocationCreateFromBitmap(type,
                                                                AllocationMipmapControl::NONE,
                                                                _data,
                                                                (int)AllocationUsageType::SCRIPT);
    // 256 x 256 x float1
    Type subType = context->typeCreate(element, 256, 256, 0, false, false, YuvFormat::YUV_NONE);
    // 256 x 256 x float1
    AllocationAdapter allocationAdapter = context->allocationAdapterCreate(subType, allocation);
    EXPECT_NE(AllocationAdapter(0), allocationAdapter);

    std::vector<uint32_t> offsets(9, 0);
    offsets[0] = 128;
    offsets[1] = 128;
    hidl_vec<uint32_t> _offsets;
    _offsets.setToExternal(offsets.data(), offsets.size());
    // origin at (128,128)
    context->allocationAdapterOffset(allocationAdapter, _offsets);

    context->allocation2DRead(allocationAdapter, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 256,
                              256, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(float), 0);
    for (int i = 128; i < 128 + 256; ++i) {
        for (int j = 128; j < 128 + 256; ++j) {
            expected.push_back(i * 512 + j);
        }
    }
    EXPECT_EQ(true, dataOut == expected);
}

TEST_F(RenderscriptHidlTest, SimpleMipmapTest) {
    // uint8_t
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    // 64 x 64 x uint8_t
    Type type = context->typeCreate(element, 64, 64, 0, true, false, YuvFormat::YUV_NONE);
    std::vector<uint8_t> dataIn(64*64), dataOut(32*32), expected(32*32);
    std::generate(dataIn.begin(), dataIn.end(),
                  [](){ static int val = 0; return (uint8_t)(0xFF & val++); });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(uint8_t));
    // 512 x 512 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::FULL,
                                                         (int)AllocationUsageType::SCRIPT,
                                                         (Ptr)nullptr);
    context->allocation2DWrite(allocation, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 64, 64,
                               _data, 64*sizeof(uint8_t));
    context->allocationGenerateMipmaps(allocation);
    context->allocationSyncAll(allocation, AllocationUsageType::SCRIPT);
    context->allocation2DRead(allocation, 0, 0, 1, AllocationCubemapFace::POSITIVE_X, 32, 32,
                              (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(uint8_t),
                              32*sizeof(uint8_t));
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            expected[i*32 + j] = ((uint32_t)dataIn[i*2*64 + j*2] + dataIn[i*2*64 + j*2 + 1] +
                                  dataIn[i*2*64 + j*2 + 64] + dataIn[i*2*64 + j*2 + 64+1]) * 0.25f;
        }
    }
    EXPECT_EQ(true, dataOut == expected);
}

TEST_F(RenderscriptHidlTest, SimpleCubemapTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x 128 x float1
    Type type = context->typeCreate(element, 128, 128, 0, false, true, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(128*128*6), dataOut(128*128), expected(128*128);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 128 x 128 x float1 x 6
    Allocation allocation = context->allocationCubeCreateFromBitmap(type,
                                                                    AllocationMipmapControl::NONE,
                                                                    _data,
                                                               (int)AllocationUsageType::SCRIPT);
    EXPECT_NE(Allocation(0), allocation);

    context->allocation2DRead(allocation, 0, 0, 0, AllocationCubemapFace::NEGATIVE_Z, 128,
                              128, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(float),
                              128*sizeof(float));
    for (int i = 0; i < 128; ++i) {
        for (int j = 0; j < 128; ++j) {
            expected[i*128 + j] = i*128*6 + j + 128*5;
        }
    }
    EXPECT_EQ(true, dataOut == expected);
}

TEST_F(RenderscriptHidlTest, MetadataTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x float1
    Type type = context->typeCreate(element, 128, 0, 0, false, false, YuvFormat::YUV_NONE);

    std::vector<uint32_t> elementMetadata(5);
    context->elementGetNativeMetadata(element, [&](const hidl_vec<uint32_t>& _metadata){
                                          elementMetadata = _metadata; });
    EXPECT_EQ(DataType::FLOAT_32, (DataType)elementMetadata[0]);
    EXPECT_EQ(DataKind::USER, (DataKind)elementMetadata[1]);
    EXPECT_EQ(false, ((uint32_t)elementMetadata[2] == 1) ? true : false);
    EXPECT_EQ(1, (uint32_t)elementMetadata[3]);
    EXPECT_EQ(0, (uint32_t)elementMetadata[4]);

    std::vector<OpaqueHandle> typeMetadata(6);
    context->typeGetNativeMetadata(type, [&typeMetadata](const hidl_vec<OpaqueHandle>& _metadata){
                                   typeMetadata = _metadata; });
    EXPECT_EQ(128, (uint32_t)typeMetadata[0]);
    EXPECT_EQ(0, (uint32_t)typeMetadata[1]);
    EXPECT_EQ(0, (uint32_t)typeMetadata[2]);
    EXPECT_EQ(false, ((uint32_t)typeMetadata[3] == 1) ? true : false);
    EXPECT_EQ(false, ((uint32_t)typeMetadata[4] == 1) ? true : false);
    EXPECT_EQ(element, (Element)typeMetadata[5]);
}

TEST_F(RenderscriptHidlTest, ResizeTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x float1
    Type type = context->typeCreate(element, 128, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Ptr dataPtr1, dataPtr2;
    Size stride;
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){
                                      dataPtr1 = _dataPtr; stride = _stride; });
    EXPECT_EQ(0, stride);

    context->allocationResize1D(allocation, 1024*1024);
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){
                                      dataPtr2 = _dataPtr; stride = _stride; });
    EXPECT_EQ(0, stride);
    EXPECT_NE(dataPtr1, dataPtr2);
}

/**
TEST_F(RenderscriptHidlTest, NativeWindowIoTest) {
    // uint8x4
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 4);
    // 512 x 512 x uint8x4
    Type type = context->typeCreate(element, 512, 512, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<uint32_t> dataIn(512*512), dataOut(512*512);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (uint32_t)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(uint32_t));
    // 512 x 512 x float1
    Allocation allocationRecv = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                               (int)(AllocationUsageType::SCRIPT
                                                               | AllocationUsageType::IO_INPUT),
                                                               (Ptr)nullptr);
    Allocation allocationSend = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                               (int)(AllocationUsageType::SCRIPT
                                                               | AllocationUsageType::IO_OUTPUT),
                                                               (Ptr)nullptr);
    context->allocation2DWrite(allocationSend, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 512, 512,
                               _data, 0);
    NativeWindow nativeWindow = context->allocationGetNativeWindow(allocationRecv);
    EXPECT_NE(NativeWindow(0), nativeWindow);

    context->allocationSetNativeWindow(allocationSend, nativeWindow); // HERE
    context->allocationIoSend(allocationSend);
    context->allocationIoReceive(allocationRecv);
    context->allocation2DRead(allocationRecv, 0, 0, 0, AllocationCubemapFace::POSITIVE_X, 512, 512,
                              (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(uint32_t), 0);
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                             [](uint32_t x){ static int val = 0; return x == (uint32_t)val++; });
    EXPECT_EQ(true, same);
}
/**
TEST_F(RenderscriptHidlTest, BufferQueueTest) {
    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 512 x 512 x float1
    Type type = context->typeCreate(element, 512, 512, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<float> dataIn(512*512), dataOut1(512*512), dataOut2(512*512);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return (float)val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(float));
    // 512 x 512 x float1
    Allocation allocationRecv1 = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                                (int)(AllocationUsageType::SCRIPT
                                                                | AllocationUsageType::IO_INPUT),
                                                                (Ptr)nullptr);
    Allocation allocationRecv2 = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                                (int)(AllocationUsageType::SCRIPT
                                                                | AllocationUsageType::IO_INPUT),
                                                                (Ptr)nullptr);
    Allocation allocationSend = context->allocationCreateFromBitmap(type,
                                                                    AllocationMipmapControl::NONE,
                                                                    _data,
                                                                   (int)(AllocationUsageType::SCRIPT
                                                                 | AllocationUsageType::IO_OUTPUT));
    context->allocationSetupBufferQueue(allocationRecv1, 2);
    context->allocationShareBufferQueue(allocationRecv1, allocationRecv2);
    // TODO: propogate data?
}
/**/

TEST_F(RenderscriptHidlTest, ContextMessageTest) {
    context->contextInitToClient();

    std::string messageOut = "correct";
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)const_cast<char*>(messageOut.c_str()), messageOut.length());
    context->contextSendMessage(0, _data);
    MessageToClientType messageType;
    size_t size;
    uint32_t subID;
    context->contextPeekMessage([&](MessageToClientType _type, Size _size, uint32_t _subID){
                                messageType = _type; size = (uint32_t)_size; subID = _subID; });
    std::vector<char> messageIn(size, '\0');
    context->contextGetMessage(messageIn.data(), messageIn.size(),
                               [&](MessageToClientType _type, Size _size){
                               messageType = _type; size = (uint32_t)_size; });
    EXPECT_EQ(true, messageOut == messageIn.data());

    context->contextDeinitToClient();
    context->contextLog();
}

TEST_F(RenderscriptHidlTest, IntrinsicTest) {
    // uint8
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    Script script = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_BLUR, element);
    EXPECT_NE(Script(0), script);

    context->scriptSetTimeZone(script, "UTF-8");
}

TEST_F(RenderscriptHidlTest, ScriptVarTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // arg tests
    context->scriptSetVarI(script, 0, 100);
    int resultI = 0;
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               resultI = *((int*)_data.data()); });
    EXPECT_EQ(100, resultI);

    context->scriptSetVarJ(script, 1, 101);
    int resultJ = 0;
    context->scriptGetVarV(script, 1, sizeof(long), [&](const hidl_vec<uint8_t>& _data){
                               resultJ = *((long*)_data.data()); });
    EXPECT_EQ(101, resultJ);

    context->scriptSetVarF(script, 2, 102.0f);
    int resultF = 0.0f;
    context->scriptGetVarV(script, 2, sizeof(float), [&](const hidl_vec<uint8_t>& _data){
                               resultF = *((float*)_data.data()); });
    EXPECT_EQ(102.0f, resultF);

    context->scriptSetVarD(script, 3, 103.0);
    int resultD = 0.0;
    context->scriptGetVarV(script, 3, sizeof(double), [&](const hidl_vec<uint8_t>& _data){
                               resultD = *((double*)_data.data()); });
    EXPECT_EQ(103.0, resultD);

    // float1
    Element element = context->elementCreate(DataType::FLOAT_32, DataKind::USER, false, 1);
    // 128 x float1
    Type type = context->typeCreate(element, 128, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x float1
    Allocation allocationIn = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                             (int)AllocationUsageType::SCRIPT,
                                                             (Ptr)nullptr);
    Allocation allocationOut = Allocation(0);
    context->scriptSetVarObj(script, 4, (ObjectBase)allocationIn);
    context->scriptGetVarV(script, 4, sizeof(ObjectBase), [&](const hidl_vec<uint8_t>& _data){
                               allocationOut = (Allocation) *((ObjectBase*)_data.data()); });
    EXPECT_EQ(allocationOut, allocationIn);

    std::vector<int> arrayIn = {500, 501, 502, 503};
    std::vector<int> arrayOut(4);
    hidl_vec<uint8_t> arrayData;
    arrayData.setToExternal((uint8_t*)arrayIn.data(), arrayIn.size()*sizeof(int));
    context->scriptSetVarV(script, 5, arrayData);
    context->scriptGetVarV(script, 5, 4*sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               arrayOut = std::vector<int>((int*)_data.data(),
                                                           (int*)_data.data() + 4); });
    EXPECT_EQ(500, arrayOut[0]);
    EXPECT_EQ(501, arrayOut[1]);
    EXPECT_EQ(502, arrayOut[2]);
    EXPECT_EQ(503, arrayOut[3]);

    std::vector<int> dataVE = {1000, 1001};
    std::vector<uint32_t> dimsVE = {1};
    std::vector<int> outVE(2);
    hidl_vec<uint8_t> _dataVE;
    hidl_vec<uint32_t> _dimsVE;
    _dataVE.setToExternal((uint8_t*)dataVE.data(), dataVE.size()*sizeof(int));
    _dimsVE.setToExternal((uint32_t*)dimsVE.data(), dimsVE.size());
    // intx2
    Element elementVE = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 2);
    context->scriptSetVarVE(script, 6, _dataVE, elementVE, _dimsVE);
    context->scriptGetVarV(script, 6, 2*sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               outVE = std::vector<int>((int*)_data.data(),
                                                        (int*)_data.data() + 2); });
    EXPECT_EQ(1000, outVE[0]);
    EXPECT_EQ(1001, outVE[1]);
}

TEST_F(RenderscriptHidlTest, ScriptInvokeTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // invoke test
    int function_res = 0;
    context->scriptInvoke(script, 0);
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               function_res = *((int*)_data.data()); });
    EXPECT_NE(100, function_res);

    // invokeV test
    int functionV_arg = 5;
    int functionV_res = 0;
    hidl_vec<uint8_t> functionV_data;
    functionV_data.setToExternal((uint8_t*)&functionV_arg, sizeof(int));
    context->scriptInvokeV(script, 1, functionV_data);
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               functionV_res = *((int*)_data.data()); });
    EXPECT_EQ(5, functionV_res);
}

TEST_F(RenderscriptHidlTest, ScriptForEachTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<uint8_t> dataIn(64), dataOut(64);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static uint8_t val = 0; return val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size());
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Allocation vout = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                     (int)AllocationUsageType::SCRIPT,
                                                     (Ptr)nullptr);
    context->allocation1DWrite(allocation, 0, 0, (Size)dataIn.size(), _data);
    hidl_vec<Allocation> vains;
    vains.setToExternal(&allocation, 1);
    hidl_vec<uint8_t> params;
    context->scriptForEach(script, 1, vains, vout, params, nullptr);
    context->allocationRead(vout, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(uint8_t));
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](uint8_t x){ static uint8_t val = 1; return x == val++; });
    EXPECT_EQ(true, same);

    for (int i = 0; i < 64; ++i)
        LOG(INFO) << "ERROR(" << i << "): " << (int)dataIn[i] << " -> " << (int)dataOut[i];
}

TEST_F(RenderscriptHidlTest, ScriptReduceTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    Type type2 = context->typeCreate(element, 1, 0, 0, false, false, YuvFormat::YUV_NONE);
    std::vector<int> dataIn(64), dataOut(1);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static int val = 0; return val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(int));
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Allocation vaout = context->allocationCreateTyped(type2, AllocationMipmapControl::NONE,
                                                      (int)AllocationUsageType::SCRIPT,
                                                      (Ptr)nullptr);
    context->allocation1DWrite(allocation, 0, 0, (Size)dataIn.size(), _data);
    hidl_vec<Allocation> vains;
    vains.setToExternal(&allocation, 1);
    context->scriptReduce(script, 0, vains, vaout, nullptr);
    context->contextFinish();
    context->allocationRead(vaout, (Ptr)dataOut.data(), (Size)dataOut.size()*sizeof(int));
    // sum of 0, 1, 2, ..., 62, 63
    int sum = 63*64/2;
    EXPECT_EQ(sum, dataOut[0]);
}

TEST_F(RenderscriptHidlTest, ScriptBindTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/", bitcode);
    EXPECT_NE(Script(0), script);

    // uint8_t
    Element element = context->elementCreate(DataType::SIGNED_32, DataKind::USER, false, 1);
    // 64 x uint8_t
    Type type = context->typeCreate(element, 64, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 64 x float1
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    Ptr dataPtr1, dataPtr2;
    Size stride;
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){ dataPtr1 = _dataPtr;
                                      stride = _stride; });
    context->scriptBindAllocation(script, allocation, 7);
    context->allocationGetPointer(allocation, 0, AllocationCubemapFace::POSITIVE_X, 0,
                                  [&](Ptr _dataPtr, Size _stride){ dataPtr2 = _dataPtr;
                                      stride = _stride; });
    EXPECT_NE(dataPtr1, dataPtr2);
}
/*
TEST_F(RenderscriptHidlTest, ScriptGroupTest) {
    //std::vector<uint8_t> dataIn(256*256*1, 128), dataOut(256*256*3, 0);
    std::vector<uint8_t> dataIn(256*256*1, 128), dataOut(256*256*4, 0);
    hidl_vec<uint8_t> _dataIn, _dataOut;
    _dataIn.setToExternal(dataIn.data(), dataIn.size());
    _dataOut.setToExternal(dataOut.data(), dataIn.size());

    // 256 x 256 YUV pixels
    Element element1 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_YUV, true, 1);
    //Type type1 = context->typeCreate(element1, 256, 256, 0, false, false, YuvFormat::YUV_420_888);
    Type type1 = context->typeCreate(element1, 256, 256, 0, false, false, YuvFormat::YUV_NV21);
    Allocation allocation1 = context->allocationCreateTyped(type1, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    context->allocation1DWrite(allocation1, 0, 0, (Size)dataIn.size(), _dataIn);
    Script yuv2rgb = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_YUV_TO_RGB, element1);
    EXPECT_NE(Script(0), yuv2rgb);

    ScriptKernelID yuv2rgbKID = context->scriptKernelIDCreate(yuv2rgb, 0, 2);
    EXPECT_NE(ScriptKernelID(0), yuv2rgbKID);

    // 256 x 256 RGB pixels
    //Element element2 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_RGB, true, 3);
    Element element2 = context->elementCreate(DataType::UNSIGNED_8, DataKind::PIXEL_RGBA, true, 4);
    Type type2 = context->typeCreate(element2, 256, 256, 0, false, false, YuvFormat::YUV_NONE);
    Allocation allocation2 = context->allocationCreateTyped(type2, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    context->allocation1DWrite(allocation2, 0, 0, (Size)dataOut.size(), _dataOut);
    Script blur = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_BLUR, element2);
    EXPECT_NE(Script(0), blur);

    ScriptKernelID blurKID = context->scriptKernelIDCreate(blur, 0, 2);
    EXPECT_NE(ScriptKernelID(0), blurKID);

    // ScriptGroup
    hidl_vec<ScriptKernelID> kernels = {yuv2rgb, blur};
    hidl_vec<ScriptKernelID> srcK = {yuv2rgb};
    hidl_vec<ScriptKernelID> dstK = {blur};
    hidl_vec<ScriptFieldID> dstF = {};
    hidl_vec<Type> types = {type2};
    ScriptGroup scriptGroup = context->scriptGroupCreate(kernels, srcK, dstK, dstF, types);
    EXPECT_NE(ScriptGroup(0), scriptGroup);

    context->scriptGroupSetInput(scriptGroup, yuv2rgbKID, allocation1);
    context->scriptGroupSetOutput(scriptGroup, blurKID, allocation2);
    //context->scriptGroupExecute(scriptGroup);

    // verify contents were changed
    context->allocation1DRead(allocation2, 0, 0, (uint32_t)dataOut.size(), (Ptr)dataOut.data(),
                              (Size)dataOut.size()*sizeof(uint8_t));
    bool same = std::all_of(dataOut.begin(), dataOut.end(), [](uint8_t x){ return x != 0; });
    EXPECT_EQ(true, same);
}
/**
TEST_F(RenderscriptHidlTest, ScriptGroup2Test) {

    ScriptFieldID fieldID = context->scriptFieldIDCreate(script, slot); // ???
    EXPECT_NE(ScriptFieldID(0), fieldID);

    ScriptKernelID kernelID = context->scriptKernelIDCreate(script, slot, sig);
    EXPECT_NE(ScriptKernelID(0), kernelID);

    Allocation returnValue = ...;
    hidl_vec<ScriptFieldID> fieldIDS = {};
    hidl_vec<int64_t> values = {};
    hidl_vec<int32_t> sizes = {};
    hidl_veC<Closure> depClosures = {};
    hidl_vec<ScriptFieldID> depFieldIDS = {};
    Closure closure1 = context->closureCreate(kernelID, returnValue, fieldIDS, values, sizes,
                                             depClosures, depFieldIDS);
    EXPECT_NE(Closure(0), closure1);

    ScriptInvokeID invokeID = context->scriptInvokeIDCreate(script, slot);
    EXPECT_NE(ScriptInvokeID(0), invokeID);

    hidl_vec<uint8_t> params = {};
    hidl_vec<ScriptFieldID> fieldsIDS2 = {};
    hidl_vec<int64_t> values2 = {};
    hidl_vec<int32_t> sizes2 = {};
    Closure closure2 = context->invokeClosureCreate(invokeID, params, fieldIDS2, values2, sizes2);
    EXPECT_NE(Closure(0), closure2);

    context->closureSetArg(closure, index, value, size);
    context->closureSetGlobal(closure, fieldID, value, size);

    hidl_string name = "script_group_2_test";
    hidl_string cacheDir = "data/local/tmp/";
    hidl_vec<Closures> closures;
    ScriptGroup2 scriptGroup2 = context->scriptGroup2Create(name, cacheDir, closures);
    EXPECT_NE(ScriptGroup2(0), scriptGroup2);

    context->scriptGroupExecute(scriptGroup2);
    // verify script group launched...
}
/**/

TEST_F(RenderscriptHidlTest, MiscellaneousTests) {
    context->contextSetPriority(ThreadPriorities::NORMAL);
    context->contextSetCacheDir("/data/local/tmp/temp/");

    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    std::string nameIn = "element_test_name";
    std::string nameOut = "not_name";
    hidl_string _nameIn;
    _nameIn.setToExternal(nameIn.c_str(), nameIn.length());
    context->assignName(element, _nameIn);
    context->contextFinish();
    context->getName(element, [&](const hidl_string& _name){ nameOut = _name.c_str(); });
    EXPECT_EQ(true, nameOut == "element_test_name");

    context->objDestroy(element);

    Sampler sampler = context->samplerCreate(SamplerValue::LINEAR, SamplerValue::LINEAR,
                                             SamplerValue::LINEAR, SamplerValue::LINEAR,
                                             SamplerValue::LINEAR, 8.0f);
    EXPECT_NE(Sampler(0), sampler);
}

TEST_F(RenderscriptHidlTest, ComplexElementTest) {
    Element element1 = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    Element element2 = context->elementCreate(DataType::UNSIGNED_32, DataKind::USER, false, 1);

    hidl_vec<Element> eins = {element1, element2};
    hidl_vec<hidl_string> names = {hidl_string("first"), hidl_string("second")};
    hidl_vec<Size> arraySizesPtr = {sizeof(uint8_t), sizeof(uint32_t)};
    Element element3 = context->elementComplexCreate(eins, names, arraySizesPtr);
    EXPECT_NE(Element(0), element3);

    std::vector<Element> ids;
    std::vector<std::string> namesOut;
    std::vector<Size> arraySizesOut;
    context->elementGetSubElements(element3, 2, [&](const hidl_vec<Element>& _ids,
                                                    const hidl_vec<hidl_string>& _names,
                                                    const hidl_vec<Size>& _arraySizes){
                                                        ids = _ids;
                                                        namesOut.push_back(_names[0]);
                                                        namesOut.push_back(_names[1]);
                                                        arraySizesOut = _arraySizes;
                                                    });
    EXPECT_NE(Element(0), ids[0]);
    EXPECT_NE(Element(0), ids[1]);
    EXPECT_EQ(true, namesOut[0] == "first");
    EXPECT_EQ(true, namesOut[1] == "second");
    EXPECT_EQ(sizeof(uint8_t), arraySizesOut[0]);
    EXPECT_EQ(sizeof(uint32_t), arraySizesOut[1]);

    // 128 x (uint8_t, uint32_t)
    Type type = context->typeCreate(element3, 128, 0, 0, false, false, YuvFormat::YUV_NONE);
    // 128 x (uint8_t, uint32_t)
    Allocation allocation = context->allocationCreateTyped(type, AllocationMipmapControl::NONE,
                                                           (int)AllocationUsageType::SCRIPT,
                                                           (Ptr)nullptr);
    std::vector<uint32_t> dataIn(128), dataOut(128);
    std::generate(dataIn.begin(), dataIn.end(), [](){ static uint32_t val = 0; return val++; });
    hidl_vec<uint8_t> _data;
    _data.setToExternal((uint8_t*)dataIn.data(), dataIn.size()*sizeof(uint32_t));
    context->allocationElementWrite(allocation, 0, 0, 0, 0, _data, 1);
    context->allocationElementRead(allocation, 0, 0, 0, 0, (Ptr)dataOut.data(),
                                   (Size)dataOut.size()*sizeof(uint32_t), 1);
    bool same = std::all_of(dataOut.begin(), dataOut.end(),
                            [](uint32_t x){ static uint32_t val = 0; return x == val++; });
    EXPECT_EQ(true, same);
}


int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(new HidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}


// API
/*
----------------------------------
P : TEST NAME
----------------------------------
X : allocationAdapterCreate
X : allocationAdapterOffset
X : allocationGetType
X : allocationCreateTyped
X : allocationCreateFromBitmap
X : allocationCubeCreateFromBitmap
X : allocationGetNativeWindow
X : allocationSetNativeWindow
X : allocationSetupBufferQueue
X : allocationShareBufferQueue
X : allocationCopyToBitmap
X : allocation1DWrite
X : allocationElementWrite
X : allocation2DWrite
X : allocation3DWrite
X : allocationGenerateMipmaps
X : allocationRead
X : allocation1DRead
X : allocationElementRead
X : allocation2DRead
X : allocation3DRead
X : allocationSyncAll
X : allocationResize1D
X : allocationCopy2DRange
X : allocationCopy3DRange
X : allocationIoSend
X : allocationIoReceive
X : allocationGetPointer
X : elementGetNativeMetadata
X : elementGetSubElements
X : elementCreate
X : elementComplexCreate
X : typeGetNativeMetadata
X : typeCreate
X : contextCreate
X : contextDestroy
X : contextGetMessage
X : contextPeekMessage
X : contextSendMessage
X : contextInitToClient
X : contextDeinitToClient
X : contextFinish
X : contextLog
X : contextSetPriority
X : contextSetCacheDir
X : assignName
X : getName
  : closureCreate
  : invokeClosureCreate
  : closureSetArg
  : closureSetGlobal
X : scriptKernelIDCreate
  : scriptInvokeIDCreate
  : scriptFieldIDCreate
X : scriptGroupCreate
  : scriptGroup2Create
X : scriptGroupSetOutput
X : scriptGroupSetInput
X : scriptGroupExecute
X : objDestroy
X : samplerCreate
X : scriptBindAllocation
X : scriptSetTimeZone
X : scriptInvoke
X : scriptInvokeV
X : scriptForEach
X : scriptReduce
X : scriptSetVarI
X : scriptSetVarObj
X : scriptSetVarJ
X : scriptSetVarF
X : scriptSetVarD
X : scriptSetVarV
X : scriptGetVarV
X : scriptSetVarVE
X : scriptCCreate
X : scriptIntrinsicCreate
---------------------------------
*/
