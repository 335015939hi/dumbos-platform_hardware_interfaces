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
#include <android-base/logging.h>

#include <android/hardware/renderscript/1.0/IContext.h>
#include <android/hardware/renderscript/1.0/IDevice.h>
#include <android/hardware/renderscript/1.0/types.h>

#include <VtsHalHidlTargetBaseTest.h>
#include <gtest/gtest.h>

using ::android::hardware::renderscript::V1_0::Allocation;
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
using ::android::hardware::renderscript::V1_0::Ptr;
using ::android::hardware::renderscript::V1_0::Size;
using ::android::hardware::renderscript::V1_0::Type;
using ::android::hardware::renderscript::V1_0::YuvFormat;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;

// The main test class for RENDERSCRIPT HIDL HAL.
class RenderscriptHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
public:
    virtual void SetUp() override {
        device = IDevice::getService();
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

/*
// get error message
if (allocation == Allocation(0)) {
    MessageToClientType messageType; size_t size; uint32_t subID;
    context->contextPeekMessage([&](MessageToClientType _type, Size _size, uint32_t _subID){
                                messageType = _type; size = (uint32_t)_size; subID = _subID; });
    std::vector<char> message(size+2, '\0');
    context->contextGetMessage(message.data(), message.size()+1,
                               [&](MessageToClientType _type, Size _size){
                               messageType = _type; size = (uint32_t)_size; });
    LOG(INFO) << "allocation message: " << message.data();
}
*/

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
  : allocationAdapterCreate
  : allocationAdapterOffset
1 : allocationGetType
1 : allocationCreateTyped
1 : allocationCreateFromBitmap
  : allocationCubeCreateFromBitmap
  : allocationGetNativeWindow
  : allocationSetNativeWindow
  : allocationSetupBufferQueue
  : allocationShareBufferQueue
1 : allocationCopyToBitmap
1 : allocation1DWrite
  : allocationElementWrite
1 : allocation2DWrite
1 : allocation3DWrite
  : allocationGenerateMipmaps
1 : allocationRead
1 : allocation1DRead
  : allocationElementRead
1 : allocation2DRead
1 : allocation3DRead
  : allocationSyncAll
  : allocationResize1D
  : allocationCopy2DRange
  : allocationCopy3DRange
  : allocationIoSend
  : allocationIoReceive
  : allocationGetPointer
  : elementGetNativeMetadata
  : elementGetSubElements
1 : elementCreate
  : elementComplexCreate
  : typeGetNativeMetadata
1 : typeCreate
N : contextCreate
N : contextDestroy
  : contextGetMessage
  : contextPeekMessage
  : contextSendMessage
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
  : scriptInvoke
  : scriptInvokeV
  : scriptForEach
  : scriptReduce
  : scriptSetVarI
  : scriptSetVarObj
  : scriptSetVarJ
  : scriptSetVarF
  : scriptSetVarD
  : scriptSetVarV
  : scriptGetVarV
  : scriptSetVarVE
  : scriptCCreate
  : scriptIntrinsicCreate
---------------------------------
*/
