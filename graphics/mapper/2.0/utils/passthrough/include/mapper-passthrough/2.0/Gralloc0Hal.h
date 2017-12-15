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

#include <mapper-hal/2.0/MapperHal.h>

struct gralloc_module_t;
struct hw_module_t;

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

class Gralloc0Hal : public virtual hal::MapperHal {
   public:
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
    virtual uint64_t getValidBufferUsageMask() const;

    static void waitFenceFd(const base::unique_fd& fenceFd, const char* logname);

    const gralloc_module_t* mModule = nullptr;
    uint8_t mMinor = 0;
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
