/*
 * Copyright 2022 The Android Open Source Project
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

#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsMapperStableC_TargetTest"

#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidl/android/hardware/graphics/allocator/AllocationResult.h>
#include <aidl/android/hardware/graphics/allocator/IAllocator.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_manager.h>
#include <android/dlext.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <vndk/hardware_buffer.h>
#include <vndksupport/linker.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <tuple>

using namespace aidl::android::hardware::graphics::allocator;
using namespace aidl::android::hardware::graphics::common;
using namespace android;
using namespace android::hardware;

typedef AIMapper_Error (*AIMapper_loadIMapperFn)(AIMapper* _Nullable* _Nonnull outImplementation);

static constexpr BufferUsage pack(const std::initializer_list<BufferUsage>& usages) {
    int64_t ret = 0;
    for (const auto u : usages) {
        ret |= static_cast<int64_t>(u);
    }
    return static_cast<BufferUsage>(ret);
}

class BufferHandle {
    AIMapper* mIMapper;
    buffer_handle_t mHandle = nullptr;

  public:
    explicit BufferHandle(AIMapper* mapper, native_handle_t* rawHandle) : mIMapper(mapper) {
        EXPECT_EQ(AIMAPPER_ERROR_NONE, mIMapper->v5.importBuffer(rawHandle, &mHandle));
    }

    explicit BufferHandle(BufferHandle&& other) { *this = std::move(other); }

    BufferHandle& operator=(BufferHandle&& other) noexcept {
        reset();
        mIMapper = other.mIMapper;
        mHandle = other.mHandle;
        other.mHandle = nullptr;
        return *this;
    }

    ~BufferHandle() { reset(); }

    constexpr explicit operator bool() const noexcept { return mHandle != nullptr; }

    buffer_handle_t operator*() const noexcept { return mHandle; }

    void reset() {
        if (mHandle != nullptr) {
            EXPECT_EQ(AIMAPPER_ERROR_NONE, mIMapper->v5.freeBuffer(mHandle));
        }
    }
};

class BufferAllocation {
    AIMapper* mIMapper;
    native_handle_t* mRawHandle;
    uint32_t mStride;
    const BufferDescriptorInfo mInfo;

  public:
    BufferAllocation(const BufferAllocation&) = delete;
    void operator=(const BufferAllocation&) = delete;

    BufferAllocation(AIMapper* mapper, native_handle_t* handle, uint32_t stride,
                     const BufferDescriptorInfo& info)
        : mIMapper(mapper), mRawHandle(handle), mStride(stride), mInfo(info) {}

    ~BufferAllocation() {
        if (mRawHandle == nullptr) return;

        native_handle_close(mRawHandle);
        native_handle_delete(mRawHandle);
    }

    uint32_t stride() const { return mStride; }

    BufferHandle import() { return BufferHandle{mIMapper, mRawHandle}; }

    const native_handle_t* rawHandle() const { return mRawHandle; }
};

class GraphicsTestsBase {
  private:
    friend class BufferAllocation;
    int32_t mIAllocatorVersion = 1;
    std::shared_ptr<IAllocator> mAllocator;
    AIMapper* mIMapper = nullptr;
    AIMapper_loadIMapperFn mIMapperLoader;

  protected:
    void Initialize(std::shared_ptr<IAllocator> allocator) {
        mAllocator = allocator;
        ASSERT_NE(nullptr, mAllocator.get()) << "failed to get allocator service";
        ASSERT_TRUE(mAllocator->getInterfaceVersion(&mIAllocatorVersion).isOk());
        ASSERT_GE(mIAllocatorVersion, 2);
        std::string mapperSuffix;
        auto status = mAllocator->getIMapperLibrarySuffix(&mapperSuffix);
        ASSERT_TRUE(status.isOk()) << "Failed to get IMapper library suffix";
        std::string lib_name = "mapper." + mapperSuffix + ".so";
        void* so = android_load_sphal_library(lib_name.c_str(), RTLD_LOCAL | RTLD_NOW);
        ASSERT_NE(nullptr, so) << "Failed to load " << lib_name;
        mIMapperLoader = (AIMapper_loadIMapperFn)dlsym(so, "AIMapper_loadIMapper");
        ASSERT_NE(nullptr, mIMapperLoader) << "AIMapper_locaIMapper missing from " << lib_name;
        ASSERT_EQ(AIMAPPER_ERROR_NONE, mIMapperLoader(&mIMapper));
        ASSERT_NE(mIMapper, nullptr);
    }

  public:
    AIMapper_loadIMapperFn getIMapperLoader() const { return mIMapperLoader; }

    std::unique_ptr<BufferAllocation> allocate(const BufferDescriptorInfo& descriptorInfo) {
        AllocationResult result;
        ::ndk::ScopedAStatus status = mAllocator->allocate2(descriptorInfo, 1, &result);
        if (!status.isOk()) {
            status_t error = status.getExceptionCode();
            if (error == EX_SERVICE_SPECIFIC) {
                error = status.getServiceSpecificError();
                EXPECT_NE(OK, error) << "Failed to set error properly";
            } else {
                EXPECT_EQ(OK, error) << "Allocation transport failure";
            }
            return nullptr;
        } else {
            return std::make_unique<BufferAllocation>(mIMapper, dupFromAidl(result.buffers[0]),
                                                      result.stride, descriptorInfo);
        }
    }

    std::unique_ptr<BufferAllocation> allocateGeneric() {
        return allocate({
                .name = {"CPU_8888"},
                .width = 64,
                .height = 64,
                .layerCount = 1,
                .format = PixelFormat::RGBA_8888,
                .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
                .reservedSize = 0,
        });
    }

    bool isSupported(const BufferDescriptorInfo& descriptorInfo) {
        bool ret = false;
        EXPECT_TRUE(mAllocator->isSupported(descriptorInfo, &ret).isOk());
        return ret;
    }

    AIMapper* mapper() const { return mIMapper; }
};

class GraphicsMapperStableCTests
    : public GraphicsTestsBase,
      public ::testing::TestWithParam<std::tuple<std::string, std::shared_ptr<IAllocator>>> {
  public:
    void SetUp() override { Initialize(std::get<1>(GetParam())); }

    void TearDown() override {}
};

TEST_P(GraphicsMapperStableCTests, AllV5CallbacksDefined) {
    ASSERT_GE(mapper()->version, AIMAPPER_VERSION_5);

    EXPECT_TRUE(mapper()->v5.importBuffer);
    EXPECT_TRUE(mapper()->v5.freeBuffer);
    EXPECT_TRUE(mapper()->v5.validateBufferSize);
    EXPECT_TRUE(mapper()->v5.getTransportSize);
    EXPECT_TRUE(mapper()->v5.lock);
    EXPECT_TRUE(mapper()->v5.unlock);
    EXPECT_TRUE(mapper()->v5.flushLockedBuffer);
    EXPECT_TRUE(mapper()->v5.rereadLockedBuffer);
    EXPECT_TRUE(mapper()->v5.getMetadata);
    EXPECT_TRUE(mapper()->v5.getStandardMetadata);
    EXPECT_TRUE(mapper()->v5.setMetadata);
    EXPECT_TRUE(mapper()->v5.setStandardMetadata);
    EXPECT_TRUE(mapper()->v5.getFromBufferDescriptorInfo);
    EXPECT_TRUE(mapper()->v5.listSupportedMetadataTypes);
    EXPECT_TRUE(mapper()->v5.dumpBuffer);
    EXPECT_TRUE(mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, DualLoadIsIdentical) {
    ASSERT_GE(mapper()->version, AIMAPPER_VERSION_5);

    EXPECT_TRUE(mapper()->v5.importBuffer);
    EXPECT_TRUE(mapper()->v5.freeBuffer);
    EXPECT_TRUE(mapper()->v5.validateBufferSize);
    EXPECT_TRUE(mapper()->v5.getTransportSize);
    EXPECT_TRUE(mapper()->v5.lock);
    EXPECT_TRUE(mapper()->v5.unlock);
    EXPECT_TRUE(mapper()->v5.flushLockedBuffer);
    EXPECT_TRUE(mapper()->v5.rereadLockedBuffer);
    EXPECT_TRUE(mapper()->v5.getMetadata);
    EXPECT_TRUE(mapper()->v5.getStandardMetadata);
    EXPECT_TRUE(mapper()->v5.setMetadata);
    EXPECT_TRUE(mapper()->v5.setStandardMetadata);
    EXPECT_TRUE(mapper()->v5.getFromBufferDescriptorInfo);
    EXPECT_TRUE(mapper()->v5.listSupportedMetadataTypes);
    EXPECT_TRUE(mapper()->v5.dumpBuffer);
    EXPECT_TRUE(mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, CanAllocate) {
    auto buffer = allocate({
            .name = {"CPU_8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);
}

TEST_P(GraphicsMapperStableCTests, ImportFreeBuffer) {
    auto buffer = allocate({
            .name = {"CPU_8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);

    {
        auto import1 = buffer->import();
        auto import2 = buffer->import();
        EXPECT_TRUE(import1);
        EXPECT_TRUE(import2);
        EXPECT_NE(*import1, *import2);
    }
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer cross mapper instances.
 */
TEST_P(GraphicsMapperStableCTests, ImportFreeBufferSingleton) {
    auto buffer = allocate({
            .name = {"CPU_8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);

    buffer_handle_t bufferHandle = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.importBuffer(buffer->rawHandle(), &bufferHandle));
    ASSERT_NE(nullptr, bufferHandle);

    AIMapper* secondMapper;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, getIMapperLoader()(&secondMapper));
    ASSERT_EQ(AIMAPPER_ERROR_NONE, secondMapper->v5.freeBuffer(bufferHandle));
}

/**
 * Test IMapper::importBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperStableCTests, ImportBufferNegative) {
    native_handle_t* invalidHandle = nullptr;
    buffer_handle_t bufferHandle = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.importBuffer(invalidHandle, &bufferHandle))
            << "importBuffer with nullptr did not fail with BAD_BUFFER";

    invalidHandle = native_handle_create(0, 0);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.importBuffer(invalidHandle, &bufferHandle))
            << "importBuffer with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(invalidHandle);
}

/**
 * Test IMapper::freeBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperStableCTests, FreeBufferNegative) {
    native_handle_t* bufferHandle = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(bufferHandle))
            << "freeBuffer with nullptr did not fail with BAD_BUFFER";

    bufferHandle = native_handle_create(0, 0);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(bufferHandle))
            << "freeBuffer with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(bufferHandle);

    auto buffer = allocateGeneric();
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(buffer->rawHandle()))
            << "freeBuffer with un-imported handle did not fail with BAD_BUFFER";
}

std::vector<std::tuple<std::string, std::shared_ptr<IAllocator>>> getIAllocatorsAtLeastVersion(
        int32_t minVersion) {
    auto instanceNames = getAidlHalInstanceNames(IAllocator::descriptor);
    std::vector<std::tuple<std::string, std::shared_ptr<IAllocator>>> filteredInstances;
    filteredInstances.reserve(instanceNames.size());
    for (const auto& name : instanceNames) {
        auto allocator =
                IAllocator::fromBinder(ndk::SpAIBinder(AServiceManager_checkService(name.c_str())));
        int32_t version = 0;
        if (allocator->getInterfaceVersion(&version).isOk()) {
            if (version >= minVersion) {
                filteredInstances.emplace_back(name, std::move(allocator));
            }
        }
    }
    return filteredInstances;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsMapperStableCTests);
INSTANTIATE_TEST_CASE_P(PerInstance, GraphicsMapperStableCTests,
                        testing::ValuesIn(getIAllocatorsAtLeastVersion(2)),
                        [](auto info) -> std::string {
                            std::string name =
                                    std::to_string(info.index) + "/" + std::get<0>(info.param);
                            return Sanitize(name);
                        });