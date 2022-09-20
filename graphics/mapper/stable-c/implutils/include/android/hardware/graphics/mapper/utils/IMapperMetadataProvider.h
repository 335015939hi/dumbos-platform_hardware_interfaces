/*
 * Copyright (C) 2022 The Android Open Source Project
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

#pragma once

#include <android/hardware/graphics/mapper/IMapper.h>

#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/ExtendableType.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/PlaneLayout.h>

#include <inttypes.h>
#include <variant>
#include <vector>

/**
 * Helpers to implement get/set metadata
 */

namespace vendor::mapper {

class IMapperStandardMetadataProvider {
    using PixelFormat = aidl::android::hardware::graphics::common::PixelFormat;
    using BufferUsage = aidl::android::hardware::graphics::common::BufferUsage;
    using ExtendableType = aidl::android::hardware::graphics::common::ExtendableType;
    using PlaneLayout = aidl::android::hardware::graphics::common::PlaneLayout;

  public:
    template <class T>
    using Result = std::variant<T, AIMapper_Error>;

  protected:
    virtual ~IMapperStandardMetadataProvider() = default;

    template <class T>
    [[nodiscard]] Result<T> notImplemented() const {
        return AIMAPPER_ERROR_UNSUPPORTED;
    }

  public:
    // ----------------------------------------------
    //  Read-only standard metadata
    // ----------------------------------------------

    [[nodiscard]] virtual Result<uint64_t> getBufferId() const {
        return notImplemented<uint64_t>();
    }
    [[nodiscard]] virtual Result<const char*> getName() const {
        return notImplemented<const char*>();
    }
    [[nodiscard]] virtual Result<uint64_t> getWidth() const { return notImplemented<uint64_t>(); }
    [[nodiscard]] virtual Result<uint64_t> getHeight() const { return notImplemented<uint64_t>(); }
    [[nodiscard]] virtual Result<uint64_t> getLayerCount() const {
        return notImplemented<uint64_t>();
    }
    [[nodiscard]] virtual Result<PixelFormat> getPixelFormatRequested() const {
        return notImplemented<PixelFormat>();
    }
    [[nodiscard]] virtual Result<uint32_t> getPixelFormatFourcc() const {
        return notImplemented<uint32_t>();
    }
    [[nodiscard]] virtual Result<uint64_t> getPixelFormatModifier() const {
        return notImplemented<uint64_t>();
    }
    [[nodiscard]] virtual Result<BufferUsage> getUsage() const {
        return notImplemented<BufferUsage>();
    }
    [[nodiscard]] virtual Result<uint64_t> getAllocationSize() const {
        return notImplemented<uint64_t>();
    }
    [[nodiscard]] virtual Result<uint64_t> getProtectedContent() const {
        return notImplemented<uint64_t>();
    }
    [[nodiscard]] virtual Result<ExtendableType> getCompression() const {
        return notImplemented<ExtendableType>();
    }
    [[nodiscard]] virtual Result<ExtendableType> getInterlaced() const {
        return notImplemented<ExtendableType>();
    }
    [[nodiscard]] virtual Result<ExtendableType> getChromaSiting() const {
        return notImplemented<ExtendableType>();
    }
    [[nodiscard]] virtual Result<std::vector<PlaneLayout>> getPlaneLayouts() const {
        return notImplemented<std::vector<PlaneLayout>>();
    }
};

}  // namespace vendor::mapper