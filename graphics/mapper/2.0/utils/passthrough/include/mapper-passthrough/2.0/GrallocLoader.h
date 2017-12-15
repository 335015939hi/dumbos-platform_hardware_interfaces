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

#pragma once

#include <memory>
#include <mutex>
#include <unordered_set>

#include <mapper-hal/2.0/MapperHal.h>

struct hw_module_t;

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

class GrallocImportedBufferPool {
   public:
    void* add(native_handle_t* bufferHandle) {
        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.insert(bufferHandle).second ? bufferHandle : nullptr;
    }

    native_handle_t* remove(void* buffer) {
        auto bufferHandle = static_cast<native_handle_t*>(buffer);

        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.erase(bufferHandle) == 1 ? bufferHandle : nullptr;
    }

    const native_handle_t* get(void* buffer) {
        auto bufferHandle = static_cast<const native_handle_t*>(buffer);

        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.count(bufferHandle) == 1 ? bufferHandle : nullptr;
    }

   private:
    std::mutex mMutex;
    std::unordered_set<const native_handle_t*> mBufferHandles;
};

class GrallocLoader {
   public:
    static IMapper* load() {
        const hw_module_t* module = loadModule();
        if (!module) {
            return nullptr;
        }
        auto hal = createHal(module);
        if (!hal) {
            return nullptr;
        }
        return createMapper(std::move(hal));
    }

    // load the gralloc module
    static const hw_module_t* loadModule();

    // return the major api version of the module
    static int getModuleMajorApiVersion(const hw_module_t* module);

    // create a MapperHal instance
    static std::unique_ptr<hal::MapperHal> createHal(const hw_module_t* module);

    // get the global imported buffer pool
    static GrallocImportedBufferPool* getImportedBufferPool();

    // create an IAllocator instance
    static IMapper* createMapper(std::unique_ptr<hal::MapperHal> hal);
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
