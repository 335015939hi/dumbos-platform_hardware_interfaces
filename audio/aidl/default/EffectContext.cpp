/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <memory>
#include "include/effect-impl/EffectTypes.h"
#define LOG_TAG "AHAL_EffectContext"
#include "effect-impl/EffectContext.h"

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::getFrameSizeInBytes;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::media::audio::common::PcmType;
using ::android::hardware::EventFlag;

namespace aidl::android::hardware::audio::effect {

EffectContext::EffectContext(size_t statusDepth, const Parameter::Common& common) {
    LOG_ALWAYS_FATAL_IF(RetCode::SUCCESS != setCommon(common), "illegalCommonParameter");

    // in/outBuffer size in float (FMQ data format defined for DataMQ)
    size_t inBufferSizeInFloat = common.input.frameCount * mInputFrameSize / sizeof(float);
    size_t outBufferSizeInFloat = common.output.frameCount * mOutputFrameSize / sizeof(float);

    // only status FMQ use the EventFlag
    mStatusMQ = std::make_shared<StatusMQ>(statusDepth, true /*configureEventFlagWord*/);
    mInputMQ = std::make_shared<DataMQ>(inBufferSizeInFloat);
    mOutputMQ = std::make_shared<DataMQ>(outBufferSizeInFloat);

    if (!mStatusMQ->isValid() || !mInputMQ->isValid() || !mOutputMQ->isValid()) {
        LOG(ERROR) << __func__ << " created invalid FMQ";
    }

    EventFlag* efGroup = nullptr;
    ::android::status_t status =
            EventFlag::createEventFlag(mStatusMQ->getEventFlagWord(), &efGroup);
    LOG_ALWAYS_FATAL_IF(status != ::android::OK || !efGroup, " create EventFlagGroup failed ");
    mEfGroup.reset(efGroup);
    LOG_FATAL_IF(!mEfGroup, "InvalidEventFlag");
    mWorkBuffer.reserve(std::max(inBufferSizeInFloat, outBufferSizeInFloat));
}

// reset buffer status by abandon input data in FMQ
void EffectContext::resetBuffer() {
    auto buffer = static_cast<float*>(mWorkBuffer.data());
    std::vector<IEffect::Status> status(mStatusMQ->availableToRead());
    if (mInputMQ) {
        mInputMQ->read(buffer, mInputMQ->availableToRead());
    }
}

void EffectContext::reopen(IEffect::OpenEffectReturn* effectRet) {
    if (!mInputMQ) {
        mInputMQ = std::make_shared<DataMQ>(mCommon.input.frameCount * mInputFrameSize /
                                            sizeof(float));
    }
    if (!mOutputMQ) {
        mOutputMQ = std::make_shared<DataMQ>(mCommon.output.frameCount * mOutputFrameSize /
                                             sizeof(float));
    }
    dupeFmq(effectRet);
}

void EffectContext::dupeFmq(IEffect::OpenEffectReturn* effectRet) {
    if (effectRet) {
        effectRet->statusMQ = mStatusMQ->dupeDesc();
        effectRet->inputDataMQ = mInputMQ->dupeDesc();
        effectRet->outputDataMQ = mOutputMQ->dupeDesc();
    }
}

float* EffectContext::getWorkBuffer() {
    return static_cast<float*>(mWorkBuffer.data());
}

std::shared_ptr<EffectContext::StatusMQ> EffectContext::getStatusFmq() const {
    return mStatusMQ;
}

std::shared_ptr<EffectContext::DataMQ> EffectContext::getInputDataFmq() const {
    return mInputMQ;
}

std::shared_ptr<EffectContext::DataMQ> EffectContext::getOutputDataFmq() const {
    return mOutputMQ;
}

size_t EffectContext::getInputFrameSize() const {
    return mInputFrameSize;
}

size_t EffectContext::getOutputFrameSize() const {
    return mOutputFrameSize;
}

int EffectContext::getSessionId() const {
    return mCommon.session;
}

int EffectContext::getIoHandle() const {
    return mCommon.ioHandle;
}

RetCode EffectContext::setOutputDevice(
        const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device) {
    mOutputDevice = device;
    return RetCode::SUCCESS;
}

std::vector<aidl::android::media::audio::common::AudioDeviceDescription>
EffectContext::getOutputDevice() {
    return mOutputDevice;
}

RetCode EffectContext::setAudioMode(const aidl::android::media::audio::common::AudioMode& mode) {
    mMode = mode;
    return RetCode::SUCCESS;
}
aidl::android::media::audio::common::AudioMode EffectContext::getAudioMode() {
    return mMode;
}

RetCode EffectContext::setAudioSource(
        const aidl::android::media::audio::common::AudioSource& source) {
    mSource = source;
    return RetCode::SUCCESS;
}

aidl::android::media::audio::common::AudioSource EffectContext::getAudioSource() {
    return mSource;
}

RetCode EffectContext::setVolumeStereo(const Parameter::VolumeStereo& volumeStereo) {
    mVolumeStereo = volumeStereo;
    return RetCode::SUCCESS;
}

Parameter::VolumeStereo EffectContext::getVolumeStereo() {
    return mVolumeStereo;
}

RetCode EffectContext::setCommon(const Parameter::Common& common) {
    LOG(VERBOSE) << __func__ << common.toString();
    auto& input = common.input;
    auto& output = common.output;

    if (input.base.format.pcm != aidl::android::media::audio::common::PcmType::FLOAT_32_BIT ||
        output.base.format.pcm != aidl::android::media::audio::common::PcmType::FLOAT_32_BIT) {
        LOG(ERROR) << __func__ << " illegal IO, input "
                   << ::android::internal::ToString(input.base.format) << ", output "
                   << ::android::internal::ToString(output.base.format);
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }

    mInputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
            input.base.format, input.base.channelMask);
    mOutputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
            output.base.format, output.base.channelMask);
    mInputChannelCount = getChannelCount(input.base.channelMask);
    mOutputChannelCount = getChannelCount(output.base.channelMask);

    if (auto ret = handleCommonDataMqUpdate(common); ret != RetCode::SUCCESS) {
        return ret;
    }
    mCommon = common;
    return RetCode::SUCCESS;
}

Parameter::Common EffectContext::getCommon() {
    LOG(VERBOSE) << __func__ << mCommon.toString();
    return mCommon;
}

EventFlag* EffectContext::getStatusEventFlag() {
    return mEfGroup.get();
}

RetCode EffectContext::handleCommonDataMqUpdate(const Parameter::Common& common) {
    bool needWake = false;
    if (mInputMQ && mCommon.input != common.input) {
        LOG(ERROR) << __func__ << " : input " << __LINE__;
        mInputMQ.reset();
        needWake = true;
    }
    if (mOutputMQ && mCommon.output != common.output) {
        LOG(ERROR) << __func__ << " : output " << __LINE__;
        mOutputMQ.reset();
        needWake = true;
    }
    if (needWake && mEfGroup) {
        LOG(DEBUG) << __func__ << " : signal client for reopen " << __LINE__ << mEfGroup.get();
        if (const auto ret = mEfGroup->wake(kEventFlagDataMqUpdate); ret != ::android::OK) {
            LOG(ERROR) << __func__ << ": wake failure with ret " << ret;
            return RetCode::ERROR_EVENT_FLAG_ERROR;
        }
    }
    return RetCode::SUCCESS;
}
}  // namespace aidl::android::hardware::audio::effect
