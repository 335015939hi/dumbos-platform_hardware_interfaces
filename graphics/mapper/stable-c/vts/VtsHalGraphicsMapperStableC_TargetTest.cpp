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
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>
#include <gralloctypes/Gralloc4.h>
#include <hidl/GtestPrinter.h>

#include <dlfcn.h>
#include <gtest/gtest.h>
#include <vndksupport/linker.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace aidl::android::hardware::graphics::allocator;
using namespace aidl::android::hardware::graphics::common;
using namespace android;
using namespace android::hardware;
using namespace ::android::hardware::graphics::mapper;

typedef AIMapper_Error (*AIMapper_loadIMapperFn)(AIMapper* _Nullable* _Nonnull outImplementation);

inline constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
    using T = std::underlying_type_t<BufferUsage>;
    return static_cast<BufferUsage>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline BufferUsage& operator|=(BufferUsage& lhs, BufferUsage rhs) {
    lhs = lhs | rhs;
    return lhs;
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
    const BufferDescriptorInfo& info() const { return mInfo; }

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
                .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
                .reservedSize = 0,
        });
    }

    bool isSupported(const BufferDescriptorInfo& descriptorInfo) {
        bool ret = false;
        EXPECT_TRUE(mAllocator->isSupported(descriptorInfo, &ret).isOk());
        return ret;
    }

    AIMapper* mapper() const { return mIMapper; }

    template <StandardMetadataType T>
    auto getStandardMetadata(const buffer_handle_t bufferHandle) {
        using Value = typename StandardMetadata<T>::value;
        std::vector<uint8_t> buffer;
        // Initial guess
        buffer.resize(512);
        int32_t sizeRequired = mapper()->v5.getStandardMetadata(
                bufferHandle, static_cast<int64_t>(T), buffer.data(), buffer.size());
        if (sizeRequired > buffer.size()) {
            buffer.resize(sizeRequired);
            sizeRequired = mapper()->v5.getStandardMetadata(bufferHandle, static_cast<int64_t>(T),
                                                            buffer.data(), buffer.size());
        }
        if (sizeRequired < 0 || sizeRequired >= buffer.size()) {
            ADD_FAILURE() << "getStandardMetadata failed, received " << sizeRequired
                          << " with buffer size " << buffer.size();
            // Generate a fail type
            return Value::decode(buffer.data(), 0);
        }
        return Value::decode(buffer.data(), sizeRequired);
    }

    void verifyRGBA8888PlaneLayouts(const std::vector<PlaneLayout>& planeLayouts) {
        ASSERT_EQ(1, planeLayouts.size());

        const auto& planeLayout = planeLayouts.front();

        ASSERT_EQ(4, planeLayout.components.size());

        int64_t offsetInBitsR = -1;
        int64_t offsetInBitsG = -1;
        int64_t offsetInBitsB = -1;
        int64_t offsetInBitsA = -1;

        for (const auto& component : planeLayout.components) {
            if (!gralloc4::isStandardPlaneLayoutComponentType(component.type)) {
                continue;
            }
            EXPECT_EQ(8, component.sizeInBits);
            if (component.type.value == gralloc4::PlaneLayoutComponentType_R.value) {
                offsetInBitsR = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_G.value) {
                offsetInBitsG = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_B.value) {
                offsetInBitsB = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_A.value) {
                offsetInBitsA = component.offsetInBits;
            }
        }

        EXPECT_EQ(0, offsetInBitsR);
        EXPECT_EQ(8, offsetInBitsG);
        EXPECT_EQ(16, offsetInBitsB);
        EXPECT_EQ(24, offsetInBitsA);

        EXPECT_EQ(0, planeLayout.offsetInBytes);
        EXPECT_EQ(32, planeLayout.sampleIncrementInBits);
        // Skip testing stride because any stride is valid
        EXPECT_LE(planeLayout.widthInSamples * planeLayout.heightInSamples * 4,
                  planeLayout.totalSizeInBytes);
        EXPECT_EQ(1, planeLayout.horizontalSubsampling);
        EXPECT_EQ(1, planeLayout.verticalSubsampling);
    }

    void fillRGBA8888(uint8_t* data, uint32_t height, size_t strideInBytes, size_t widthInBytes) {
        for (uint32_t y = 0; y < height; y++) {
            memset(data, y, widthInBytes);
            data += strideInBytes;
        }
    }

    void verifyRGBA8888(const buffer_handle_t bufferHandle, const uint8_t* data, uint32_t height,
                        size_t strideInBytes, size_t widthInBytes) {
        auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(bufferHandle);
        ASSERT_TRUE(decodeResult.has_value());
        const auto& planeLayouts = *decodeResult;
        ASSERT_TRUE(planeLayouts.size() > 0);

        verifyRGBA8888PlaneLayouts(planeLayouts);

        for (uint32_t y = 0; y < height; y++) {
            for (size_t i = 0; i < widthInBytes; i++) {
                EXPECT_EQ(static_cast<uint8_t>(y), data[i]);
            }
            data += strideInBytes;
        }
    }
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
    EXPECT_TRUE(mapper()->v5.getTransportSize);
    EXPECT_TRUE(mapper()->v5.lock);
    EXPECT_TRUE(mapper()->v5.unlock);
    EXPECT_TRUE(mapper()->v5.flushLockedBuffer);
    EXPECT_TRUE(mapper()->v5.rereadLockedBuffer);
    EXPECT_TRUE(mapper()->v5.getMetadata);
    EXPECT_TRUE(mapper()->v5.getStandardMetadata);
    EXPECT_TRUE(mapper()->v5.setMetadata);
    EXPECT_TRUE(mapper()->v5.setStandardMetadata);
    EXPECT_TRUE(mapper()->v5.listSupportedMetadataTypes);
    EXPECT_TRUE(mapper()->v5.dumpBuffer);
    EXPECT_TRUE(mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, DualLoadIsIdentical) {
    ASSERT_GE(mapper()->version, AIMAPPER_VERSION_5);
    AIMapper* secondMapper;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, getIMapperLoader()(&secondMapper));

    EXPECT_EQ(secondMapper->v5.importBuffer, mapper()->v5.importBuffer);
    EXPECT_EQ(secondMapper->v5.freeBuffer, mapper()->v5.freeBuffer);
    EXPECT_EQ(secondMapper->v5.getTransportSize, mapper()->v5.getTransportSize);
    EXPECT_EQ(secondMapper->v5.lock, mapper()->v5.lock);
    EXPECT_EQ(secondMapper->v5.unlock, mapper()->v5.unlock);
    EXPECT_EQ(secondMapper->v5.flushLockedBuffer, mapper()->v5.flushLockedBuffer);
    EXPECT_EQ(secondMapper->v5.rereadLockedBuffer, mapper()->v5.rereadLockedBuffer);
    EXPECT_EQ(secondMapper->v5.getMetadata, mapper()->v5.getMetadata);
    EXPECT_EQ(secondMapper->v5.getStandardMetadata, mapper()->v5.getStandardMetadata);
    EXPECT_EQ(secondMapper->v5.setMetadata, mapper()->v5.setMetadata);
    EXPECT_EQ(secondMapper->v5.setStandardMetadata, mapper()->v5.setStandardMetadata);
    EXPECT_EQ(secondMapper->v5.listSupportedMetadataTypes, mapper()->v5.listSupportedMetadataTypes);
    EXPECT_EQ(secondMapper->v5.dumpBuffer, mapper()->v5.dumpBuffer);
    EXPECT_EQ(secondMapper->v5.getReservedRegion, mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, CanAllocate) {
    auto buffer = allocate({
            .name = {"CPU_8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
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
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
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
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
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

/**
 * Test IMapper::lock and IMapper::unlock.
 */
TEST_P(GraphicsMapperStableCTests, LockUnlockBasic) {
    constexpr auto usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN;
    auto buffer = allocate({
            .name = {"CPU_8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = usage,
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());

    // lock buffer for writing
    const auto& info = buffer->info();
    const auto stride = buffer->stride();
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.lock(*handle, static_cast<int64_t>(usage), region, -1, (void**)&data));

    // RGBA_8888
    fillRGBA8888(data, info.height, stride * 4, info.width * 4);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));

    // lock again for reading
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(usage), region,
                                                     releaseFence, (void**)&data));
    releaseFence = -1;

    ASSERT_NO_FATAL_FAILURE(verifyRGBA8888(*handle, data, info.height, stride * 4, info.width * 4));

    releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
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