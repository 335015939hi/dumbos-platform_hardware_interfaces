/*
 * Copyright 2017 The Android Open Source Project
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

#include <mapper-passthrough/2.0/GrallocLoader.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <mapper-hal/2.0/Mapper.h>
#include <mapper-hal/2.0/MapperHal.h>
#include <mapper-passthrough/2.0/Gralloc0Hal.h>
#include <mapper-passthrough/2.0/Gralloc1Hal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

namespace {

// GraphicBufferMapper in framework is expected to be valid (and leaked)
// during process termination.  We need to make sure IMapper, and in turn,
// sImportedBufferPool is valid as well.  Create imported buffer pool
// statically and on the heap for the purpose, and let it leak for simplicity.
// Besides, all IMapper instances should share the name pool anyway.
//
// However, there is no way to make sure gralloc0/gralloc1 are valid during
// process termination.  Any use of static/global object in gralloc0/gralloc1
// that may have been destructed is potentially broken.
GrallocImportedBufferPool* sImportedBufferPool = new GrallocImportedBufferPool;

class GrallocMapper : public hal::Mapper {
   protected:
    void* addImportedBuffer(native_handle_t* bufferHandle) override {
        return GrallocLoader::getImportedBufferPool()->add(bufferHandle);
    }

    native_handle_t* removeImportedBuffer(void* buffer) override {
        return GrallocLoader::getImportedBufferPool()->remove(buffer);
    }

    const native_handle_t* getImportedBuffer(void* buffer) const override {
        return GrallocLoader::getImportedBufferPool()->get(buffer);
    }
};

}  // namespace

const hw_module_t* GrallocLoader::loadModule() {
    const hw_module_t* module;
    int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (error) {
        ALOGE("failed to get gralloc module");
        return nullptr;
    }

    return module;
}

int GrallocLoader::getModuleMajorApiVersion(const hw_module_t* module) {
    return (module->module_api_version >> 8) & 0xff;
}

std::unique_ptr<hal::MapperHal> GrallocLoader::createHal(const hw_module_t* module) {
    int major = getModuleMajorApiVersion(module);
    switch (major) {
        case 1: {
            auto hal = std::make_unique<Gralloc1Hal>();
            return hal->initWithModule(module) ? std::move(hal) : nullptr;
        }
        case 0: {
            auto hal = std::make_unique<Gralloc0Hal>();
            return hal->initWithModule(module) ? std::move(hal) : nullptr;
        }
        default:
            ALOGE("unknown gralloc module major version %d", major);
            return nullptr;
    }
}

GrallocImportedBufferPool* GrallocLoader::getImportedBufferPool() {
    return sImportedBufferPool;
}

IMapper* GrallocLoader::createMapper(std::unique_ptr<hal::MapperHal> hal) {
    auto mapper = std::make_unique<GrallocMapper>();
    return mapper->init(std::move(hal)) ? mapper.release() : nullptr;
}

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
