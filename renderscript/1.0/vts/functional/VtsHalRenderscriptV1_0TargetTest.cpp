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

#include <gtest/gtest.h>
#include <chrono>

using ::android::hardware::renderscript::V1_0::IContext;
using ::android::hardware::renderscript::V1_0::IDevice;
using ::android::hardware::renderscript::V1_0::ContextType;
using ::android::hardware::renderscript::V1_0::DataType;
using ::android::hardware::renderscript::V1_0::DataKind;
using ::android::hardware::renderscript::V1_0::Element;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;

// The main test class for RENDERSCRIPT HIDL HAL.
class RenderscriptHidlTest : public ::testing::Test {
public:
    virtual void SetUp() override {
        device = IDevice::getService();
        ASSERT_NE(device, nullptr);

        uint32_t version = 0;
        uint32_t flags = 0;
        context = device->contextCreate(version, ContextType::NORMAL, flags);
        ASSERT_NE(context, nullptr);
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
TEST_F(RenderscriptHidlTest, ContextCreateAndDestroy) {
}

/*
 *
 */
TEST_F(RenderscriptHidlTest, ElementCreate) {
    DataType dt = DataType::FLOAT_32;
    DataKind dk = DataKind::USER;
    bool norm = false;
    uint32_t size = 1;
    Element element = context->elementCreate(dt, dk, norm, size);
    EXPECT_NE(element, Element(0));
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(new HidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}


// API
/**
allocationAdapterCreate
allocationAdapterOffset
allocationGetType
allocationCreateTyped
allocationCreateFromBitmap
allocationCubeCreateFromBitmap
allocationGetNativeWindow
allocationSetNativeWindow
allocationSetupBufferQueue
allocationShareBufferQueue
allocationCopyToBitmap
allocation1DWrite
allocationElementWrite
allocation2DWrite
allocation3DWrite
allocationGenerateMipmaps
allocationRead
allocation1DRead
allocationElementRead
allocation2DRead
allocation3DRead
allocationSyncAll
allocationResize1D
allocationCopy2DRange
allocationCopy3DRange
allocationIoSend
allocationIoReceive
allocationGetPointer
elementGetNativeMetadata
elementGetSubElements
elementCreate
elementComplexCreate
typeGetNativeMetadata
typeCreate
contextDestroy
contextGetMessage
contextPeekMessage
contextSendMessage
contextInitToClient
contextDeinitToClient
contextFinish
contextLog
contextSetPriority
contextSetCacheDir
assignName
getName
closureCreate
invokeClosureCreate
closureSetArg
closureSetGlobal
scriptKernelIDCreate
scriptInvokeIDCreate
scriptFieldIDCreate
scriptGroupCreate
scriptGroup2Create
scriptGroupSetOutput
scriptGroupSetInput
scriptGroupExecute
objDestroy
samplerCreate
scriptBindAllocation
scriptSetTimeZone
scriptInvoke
scriptInvokeV
scriptForEach
scriptReduce
scriptSetVarI
scriptSetVarObj
scriptSetVarJ
scriptSetVarF
scriptSetVarD
scriptSetVarV
scriptGetVarV
scriptSetVarVE
scriptCCreate
scriptIntrinsicCreate
/**/
