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

// include tests files
#include "VtsCopyTests.cpp"
#include "VtsScriptTests.cpp"
#include "VtsMiscellaneousTests.cpp"


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
