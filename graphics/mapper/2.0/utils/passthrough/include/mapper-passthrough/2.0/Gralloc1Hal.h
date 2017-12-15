/*
 * Copyright 2016 The Android Open Source Project
 * * Licensed under the Apache License, Version 2.0 (the "License");
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

#include <hardware/gralloc1.h>
#include <mapper-hal/2.0/MapperHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

class Gralloc1Hal : public virtual hal::MapperHal {
   public:
    ~Gralloc1Hal();
    bool initWithModule(const hw_module_t* module);

    Error createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo,
                           BufferDescriptor* outDescriptor) override;

    Error importBuffer(const native_handle_t* rawHandle,
                       native_handle_t** outBufferHandle) override;
    Error freeBuffer(native_handle_t* bufferHandle) override;

    Error lockBuffer(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                     const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                     void** outData) override;
    Error lockBuffer(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                     const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                     YCbCrLayout* outLayout) override;
    Error unlockBuffer(const native_handle_t* bufferHandle, base::unique_fd* outFenceFd) override;

   protected:
    template <typename T>
    bool initDispatchFunction(gralloc1_function_descriptor_t desc, T* outPfn) {
        auto pfn = getDispatchFunction(desc);
        if (pfn) {
            *outPfn = reinterpret_cast<T>(pfn);
            return true;
        } else {
            return false;
        }
    }
    gralloc1_function_pointer_t getDispatchFunction(gralloc1_function_descriptor_t desc) const;

    virtual void initCapabilities();
    virtual bool initDispatch();

    virtual uint64_t getValidBufferUsageMask() const;

    static Error toError(int32_t error);
    static bool toYCbCrLayout(const android_flex_layout& flex, YCbCrLayout* outLayout);
    static gralloc1_rect_t asGralloc1Rect(const IMapper::Rect& rect);

    gralloc1_device_t* mDevice = nullptr;

    struct {
        bool layeredBuffers;
        bool releaseImplyDelete;
    } mCapabilities = {};

    struct {
        GRALLOC1_PFN_RETAIN retain;
        GRALLOC1_PFN_RELEASE release;
        GRALLOC1_PFN_GET_NUM_FLEX_PLANES getNumFlexPlanes;
        GRALLOC1_PFN_LOCK lock;
        GRALLOC1_PFN_LOCK_FLEX lockFlex;
        GRALLOC1_PFN_UNLOCK unlock;
    } mDispatch = {};
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
