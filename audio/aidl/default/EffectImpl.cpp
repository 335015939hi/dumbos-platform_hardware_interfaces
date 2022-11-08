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

#include <future>
#define LOG_TAG "AHAL_EffectImpl"
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectTypes.h"
#include "include/effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

ndk::ScopedAStatus EffectImpl::open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) {
    LOG(DEBUG) << __func__;
    {
        std::lock_guard lg(mMutex);
        RETURN_OK_IF(mState != State::INIT);
        RETURN_IF(!createContext_l(common), EX_NULL_POINTER, "createContextFailed");
    }

    RETURN_IF_ASTATUS_NOT_OK(setParameterCommon(common), "setCommParamErr");
    if (specific.has_value()) {
        RETURN_IF_ASTATUS_NOT_OK(setParameterSpecific(specific.value()), "setSpecParamErr");
    }

    RETURN_IF(createThread(getEffectName()) != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");

    {
        std::lock_guard lg(mMutex);
        auto context = getContext_l();
        RETURN_IF(!context, EX_NULL_POINTER, "nullContext");
        context->dupeFmq(ret);
        mState = State::IDLE;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::close() {
    std::lock_guard lg(mMutex);
    RETURN_OK_IF(mState == State::INIT);
    RETURN_IF(mState == State::PROCESSING, EX_ILLEGAL_STATE, "closeAtProcessing");

    // stop the worker thread, ignore the return code
    RETURN_IF(destroyThread() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToDestroyWorker");
    mState = State::INIT;
    RETURN_IF(releaseContext_l() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");

    LOG(DEBUG) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameter(const Parameter& param) {
    LOG(DEBUG) << __func__ << " with: " << param.toString();

    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
        case Parameter::deviceDescription:
        case Parameter::mode:
        case Parameter::source:
            FALLTHROUGH_INTENDED;
        case Parameter::volumeStereo:
            return setParameterCommon(param);
        case Parameter::specific: {
            return setParameterSpecific(param.get<Parameter::specific>());
        }
        default: {
            LOG(ERROR) << __func__ << " unsupportedParameterTag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ParameterNotSupported");
        }
    }
}

ndk::ScopedAStatus EffectImpl::getParameter(const Parameter::Id& id, Parameter* param) {
    LOG(DEBUG) << __func__ << id.toString();
    auto tag = id.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            RETURN_IF_ASTATUS_NOT_OK(getParameterCommon(id.get<Parameter::Id::commonTag>(), param),
                                     "CommonParamNotSupported");
            break;
        }
        case Parameter::Id::vendorEffectTag: {
            LOG(DEBUG) << __func__ << " noop for vendor tag";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "vendortagNotSupported");
        }
        default: {
            Parameter::Specific specific;
            RETURN_IF_ASTATUS_NOT_OK(getParameterSpecific(id, &specific), "SpecParamNotSupported");
            param->set<Parameter::specific>(specific);
            break;
        }
    }
    LOG(DEBUG) << __func__ << param->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameterCommon(const Parameter& param) {
    std::shared_ptr<EffectContext> context;
    {
        std::lock_guard lg(mMutex);
        context = getContext_l();
        RETURN_IF(!context, EX_NULL_POINTER, "nullContext");
    }

    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
            RETURN_IF(context->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCommFailed");
            break;
        case Parameter::deviceDescription:
            RETURN_IF(context->setOutputDevice(param.get<Parameter::deviceDescription>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            break;
        case Parameter::mode:
            RETURN_IF(context->setAudioMode(param.get<Parameter::mode>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setModeFailed");
            break;
        case Parameter::source:
            RETURN_IF(context->setAudioSource(param.get<Parameter::source>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setSourceFailed");
            break;
        case Parameter::volumeStereo:
            RETURN_IF(context->setVolumeStereo(param.get<Parameter::volumeStereo>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setVolumeStereoFailed");
            break;
        default: {
            LOG(ERROR) << __func__ << " unsupportedParameterTag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commonParamNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getParameterCommon(const Parameter::Tag& tag, Parameter* param) {
    std::shared_ptr<EffectContext> context;
    {
        std::lock_guard lg(mMutex);
        context = getContext_l();
        RETURN_IF(!context, EX_NULL_POINTER, "nullContext");
    }

    switch (tag) {
        case Parameter::common: {
            param->set<Parameter::common>(context->getCommon());
            break;
        }
        case Parameter::deviceDescription: {
            param->set<Parameter::deviceDescription>(context->getOutputDevice());
            break;
        }
        case Parameter::mode: {
            param->set<Parameter::mode>(context->getAudioMode());
            break;
        }
        case Parameter::source: {
            param->set<Parameter::source>(context->getAudioSource());
            break;
        }
        case Parameter::volumeStereo: {
            param->set<Parameter::volumeStereo>(context->getVolumeStereo());
            break;
        }
        default: {
            LOG(DEBUG) << __func__ << " unsupported tag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "tagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getState(State* state) {
    std::lock_guard lg(mMutex);
    *state = mState;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::command(CommandId command) {
    std::lock_guard lg(mMutex);
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "CommandStateError");
    std::shared_ptr<EffectContext> context = getContext_l();
    RETURN_IF(!context, EX_NULL_POINTER, "nullContext");
    LOG(DEBUG) << __func__ << ": receive command: " << toString(command) << " at state "
               << toString(mState);

    switch (command) {
        case CommandId::START:
            RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");
            RETURN_OK_IF(mState == State::PROCESSING);
            RETURN_IF_ASTATUS_NOT_OK(commandStart_l(), "commandStartFailed");
            mState = State::PROCESSING;
            startThread();
            break;
        case CommandId::STOP:
            RETURN_OK_IF(mState == State::IDLE);
            mState = State::IDLE;
            RETURN_IF_ASTATUS_NOT_OK(commandStop_l(), "commandStopFailed");
            stopThread();
            break;
        case CommandId::RESET:
            RETURN_OK_IF(mState == State::IDLE);
            mState = State::IDLE;
            RETURN_IF_ASTATUS_NOT_OK(commandStop_l(), "commandStopFailed");
            stopThread();
            context->resetBuffer();
            break;
        default:
            LOG(ERROR) << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    LOG(DEBUG) << __func__ << " transfer to state: " << toString(mState);
    return ndk::ScopedAStatus::ok();
}

void EffectImpl::cleanUp() {
    command(CommandId::STOP);
    close();
}

IEffect::Status EffectImpl::status(binder_status_t status, size_t consumed, size_t produced) {
    IEffect::Status ret;
    ret.status = status;
    ret.fmqConsumed = consumed;
    ret.fmqProduced = produced;
    return ret;
}

void EffectImpl::process() {
    auto context = getContext();
    RETURN_VALUE_IF(!context, void(), "nullContext");
    std::shared_ptr<EffectContext::StatusMQ> statusMQ = context->getStatusFmq();
    std::shared_ptr<EffectContext::DataMQ> inputMQ = context->getInputDataFmq();
    std::shared_ptr<EffectContext::DataMQ> outputMQ = context->getOutputDataFmq();

    // Only this worker will read from input data MQ and write to output data MQ.
    auto readSamples = inputMQ->availableToRead(), writeSamples = outputMQ->availableToWrite();
    if (readSamples && writeSamples) {
        auto processSamples = std::min(readSamples, writeSamples);
        LOG(DEBUG) << __func__ << " available to read " << readSamples << " available to write "
                   << writeSamples << " process " << processSamples;

        auto buffer = context->getWorkBuffer();
        inputMQ->read(buffer, processSamples);

        IEffect::Status status = effectProcessImpl(buffer, buffer, processSamples);
        outputMQ->write(buffer, status.fmqProduced);
        statusMQ->writeBlocking(&status, 1);
        LOG(DEBUG) << __func__ << " done processing, effect consumed " << status.fmqConsumed
                   << " produced " << status.fmqProduced;
    } else {
        // TODO: maybe add some sleep here to avoid busy waiting
    }
}

}  // namespace aidl::android::hardware::audio::effect
