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
using ::android::hardware::renderscript::V1_0::OpaqueHandle;
using ::android::hardware::renderscript::V1_0::Ptr;
using ::android::hardware::renderscript::V1_0::Script;
using ::android::hardware::renderscript::V1_0::ScriptIntrinsicID;
using ::android::hardware::renderscript::V1_0::Size;
using ::android::hardware::renderscript::V1_0::Type;
using ::android::hardware::renderscript::V1_0::YuvFormat;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
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
        //context->contextDestroy();
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

/**/
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
}

TEST_F(RenderscriptHidlTest, IntrinsicTest) {
    // uint8
    Element element = context->elementCreate(DataType::UNSIGNED_8, DataKind::USER, false, 1);
    Script script = context->scriptIntrinsicCreate(ScriptIntrinsicID::ID_BLUR, element);
    EXPECT_NE(Script(0), script);

    context->scriptSetVarF(script, 0, 5.0f);
    float result = 0.0f;
    //context->scriptGetVarV(script, 0, sizeof(float), [&](const hidl_vec<uint8_t>& _data){
    //                           result = *((float*)_data.data()); });
    //EXPECT_EQ(5.0f, result);
}

TEST_F(RenderscriptHidlTest, ScriptTest) {
    hidl_vec<uint8_t> bitcode;
    bitcode.setToExternal((uint8_t*)bitCode64, bitCode64Length); // change to 32 bit?
    Script script = context->scriptCCreate("struct_test", "/data/local/tmp/64/", bitcode);
    EXPECT_NE(Script(0), script);

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

    context->scriptInvoke(script, 0);
    context->scriptGetVarV(script, 0, sizeof(int), [&](const hidl_vec<uint8_t>& _data){
                               resultI = *((int*)_data.data()); });
    EXPECT_NE(100, resultI);

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
1 : allocationAdapterCreate
1 : allocationAdapterOffset
1 : allocationGetType
1 : allocationCreateTyped
1 : allocationCreateFromBitmap
1 : allocationCubeCreateFromBitmap
1 : allocationGetNativeWindow
1 : allocationSetNativeWindow
1 : allocationSetupBufferQueue
1 : allocationShareBufferQueue
1 : allocationCopyToBitmap
1 : allocation1DWrite
  : allocationElementWrite
1 : allocation2DWrite
1 : allocation3DWrite
1 : allocationGenerateMipmaps
1 : allocationRead
1 : allocation1DRead
  : allocationElementRead
1 : allocation2DRead
1 : allocation3DRead
  : allocationSyncAll
1 : allocationResize1D
1 : allocationCopy2DRange
1 : allocationCopy3DRange
1 : allocationIoSend
1 : allocationIoReceive
1 : allocationGetPointer
1 : elementGetNativeMetadata
  : elementGetSubElements
1 : elementCreate
  : elementComplexCreate
1 : typeGetNativeMetadata
1 : typeCreate
N : contextCreate
N : contextDestroy
1 : contextGetMessage
1 : contextPeekMessage
1 : contextSendMessage
  : contextInitToClient
  : contextDeinitToClient
  : contextFinish
  : contextLog
  : contextSetPriority
  : contextSetCacheDir
  : assignName
  : getName
  : closureCreate
  : invokeClosureCreate
  : closureSetArg
  : closureSetGlobal
  : scriptKernelIDCreate
  : scriptInvokeIDCreate
  : scriptFieldIDCreate
  : scriptGroupCreate
  : scriptGroup2Create
  : scriptGroupSetOutput
  : scriptGroupSetInput
  : scriptGroupExecute
  : objDestroy
  : samplerCreate
  : scriptBindAllocation
  : scriptSetTimeZone
1 : scriptInvoke
  : scriptInvokeV
  : scriptForEach
  : scriptReduce
1 : scriptSetVarI
  : scriptSetVarObj
1 : scriptSetVarJ
1 : scriptSetVarF
1 : scriptSetVarD
  : scriptSetVarV
  : scriptGetVarV
  : scriptSetVarVE
1 : scriptCCreate
1 : scriptIntrinsicCreate
---------------------------------
*/
