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
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>

namespace aidl::android::hardware::audio::effect {

class EffectContext {
  public:
    typedef ::android::AidlMessageQueue<
            IEffect::Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            float, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    EffectContext(size_t statusDepth, size_t inBufferSize, size_t outBufferSize) {
        mStatusMQ = std::make_shared<StatusMQ>(statusDepth, true /*configureEventFlagWord*/);
        mInputMQ = std::make_shared<DataMQ>(inBufferSize);
        mOutputMQ = std::make_shared<DataMQ>(outBufferSize);

        if (!mStatusMQ->isValid() || !mInputMQ->isValid() || !mOutputMQ->isValid()) {
            LOG(ERROR) << __func__ << " created invalid FMQ";
        }
        mWorkBuffer.reserve(std::max(inBufferSize, outBufferSize));
    }

    std::shared_ptr<StatusMQ> getStatusFmq() { return mStatusMQ; }
    std::shared_ptr<DataMQ> getInputDataFmq() { return mInputMQ; }
    std::shared_ptr<DataMQ> getOutputDataFmq() { return mOutputMQ; }

    float* getWorkBuffer() { return static_cast<float*>(mWorkBuffer.data()); }
    // TODO: update with actual available size
    size_t availableToRead() { return mWorkBuffer.capacity(); }
    size_t availableToWrite() { return mWorkBuffer.capacity(); }

    // reset buffer status by abandon all data and status in FMQ
    void reset() {
        auto buffer = getWorkBuffer();
        std::vector<IEffect::Status> status(mStatusMQ->availableToRead());
        mInputMQ->readBlocking(buffer, mInputMQ->availableToRead());
        mOutputMQ->readBlocking(buffer, mOutputMQ->availableToRead());
        mStatusMQ->readBlocking(status.data(), mStatusMQ->availableToRead());
    }

  private:
    std::shared_ptr<StatusMQ> mStatusMQ;
    std::shared_ptr<DataMQ> mInputMQ;
    std::shared_ptr<DataMQ> mOutputMQ;
    // TODO handle effect process input and output
    // work buffer set by effect instances, the access and update are in same thread
    std::vector<float> mWorkBuffer;
};
}  // namespace aidl::android::hardware::audio::effect
