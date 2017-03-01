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


