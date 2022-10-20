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

#define LOG_TAG "AHAL_EqualizerSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "equalizer-impl/EqualizerSw.h"

using android::hardware::audio::common::getFrameSizeInBytes;

namespace aidl::android::hardware::audio::effect {

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != EqualizerSwImplUUID) {
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<EqualizerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t destroyEffect(const std::shared_ptr<IEffect>& instanceSp) {
    State state;
    ndk::ScopedAStatus status = instanceSp->getState(&state);
    if (!status.isOk() || State::INIT != state) {
        LOG(ERROR) << __func__ << " instance " << instanceSp.get()
                   << " in state: " << toString(state) << ", status: " << status.getDescription();
        return EX_ILLEGAL_STATE;
    }
    LOG(DEBUG) << __func__ << " instance " << instanceSp.get() << " destroyed";
    return EX_NONE;
}

ndk::ScopedAStatus EqualizerSw::open(const Parameter::Common& common,
                                     const Parameter::Specific& specific,
                                     OpenEffectReturn* _aidl_return) {
    LOG(DEBUG) << __func__;
    if (mState != State::INIT) {
        LOG(WARNING) << __func__ << " eq already open";
        return ndk::ScopedAStatus::ok();
    }

    // Set essential parameters before create worker thread.
    setCommonParameter(common);
    setSpecificParameter(specific);

    LOG(DEBUG) << " common: " << common.toString() << " specific " << specific.toString();

    auto& input = common.input;
    auto& output = common.output;
    size_t inputFrameSize = getFrameSizeInBytes(input.base.format, input.base.channelMask);
    size_t outputFrameSize = getFrameSizeInBytes(output.base.format, output.base.channelMask);
    mContext = std::make_shared<EqualizerSwContext>(1, input.frameCount * inputFrameSize,
                                                    output.frameCount * outputFrameSize);
    if (!mContext) {
        LOG(ERROR) << __func__ << " created EqualizerSwContext failed";
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                                "FailedToCreateFmq");
    }
    setContext(mContext);

    // create the worker thread
    if (RetCode::SUCCESS != createThread(LOG_TAG)) {
        LOG(ERROR) << __func__ << " created worker thread failed";
        mContext.reset();
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                                "FailedToCreateWorker");
    }

    _aidl_return->statusMQ = mContext->getStatusFmq()->dupeDesc();
    _aidl_return->inputDataMQ = mContext->getInputDataFmq()->dupeDesc();
    _aidl_return->outputDataMQ = mContext->getOutputDataFmq()->dupeDesc();
    mState = State::IDLE;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::close() {
    if (mState == State::INIT) {
        LOG(WARNING) << __func__ << " instance already closed";
        return ndk::ScopedAStatus::ok();
    } else if (mState == State::PROCESSING) {
        LOG(ERROR) << __func__ << " instance still processing";
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE,
                                                                "EqInstanceProcessing");
    }

    // stop the worker thread
    mState = State::INIT;
    destroyThread();
    mContext.reset();

    LOG(DEBUG) << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << mDesc.toString();
    *_aidl_return = mDesc;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::command(CommandId in_commandId) {
    LOG(DEBUG) << __func__ << ": receive command:" << toString(in_commandId);
    if (mState == State::INIT) {
        LOG(ERROR) << __func__ << ": instance not open yet";
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE,
                                                                "CommandStateError");
    }
    switch (in_commandId) {
        case CommandId::START:
            // start processing.
            mState = State::PROCESSING;
            startThread();
            LOG(DEBUG) << __func__ << " state: " << toString(mState);
            return ndk::ScopedAStatus::ok();
        case CommandId::STOP:
            // stop processing.
            mState = State::IDLE;
            stopThread();
            LOG(DEBUG) << __func__ << " state: " << toString(mState);
            return ndk::ScopedAStatus::ok();
        case CommandId::RESET:
            mState = State::IDLE;
            stopThread();
            LOG(DEBUG) << __func__ << " state: " << toString(mState);
            return ndk::ScopedAStatus::ok();
        default:
            LOG(ERROR) << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::setParameter(const Parameter& in_param) {
    if (mState == State::INIT) {
        LOG(ERROR) << __func__ << ": instance not open yet";
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, "StateError");
    }
    LOG(DEBUG) << __func__ << " with: " << in_param.toString();
    auto tag = in_param.getTag();
    switch (tag) {
        case Parameter::common: {
            return setCommonParameter(in_param.get<Parameter::common>());
        }
        case Parameter::specific: {
            return setSpecificParameter(in_param.get<Parameter::specific>());
        }
        case Parameter::device: {
            mDevice = in_param.get<Parameter::device>();
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::mode: {
            mMode = in_param.get<Parameter::mode>();
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::source: {
            mSource = in_param.get<Parameter::source>();
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::volume: {
            mVolume = in_param.get<Parameter::volume>();
            return ndk::ScopedAStatus::ok();
        }
        default:
            LOG(ERROR) << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ParameterNotSupported");
    }
}

ndk::ScopedAStatus EqualizerSw::getParameter(const Parameter::Id& in_paramId,
                                             Parameter* _aidl_return) {
    LOG(DEBUG) << __func__ << in_paramId.toString();
    auto tag = in_paramId.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            ndk::ScopedAStatus status =
                    getCommonParameter(in_paramId.get<Parameter::Id::commonTag>(), _aidl_return);
            if (!status.isOk()) {
                LOG(ERROR) << __func__
                           << " getSpecificParameter error: " << status.getDescription();
                return status;
            }
            LOG(DEBUG) << __func__ << " get: " << _aidl_return->toString();
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::Id::equalizerTag: {
            auto& id = in_paramId.get<Parameter::Id::equalizerTag>();
            Parameter::Specific specific;
            ndk::ScopedAStatus status = getSpecificParameter(id, &specific);
            if (!status.isOk()) {
                LOG(ERROR) << __func__
                           << " getSpecificParameter error: " << status.getDescription();
                return status;
            }
            _aidl_return->set<Parameter::specific>(specific);
            LOG(DEBUG) << __func__ << _aidl_return->toString();
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::Id::vendorEffectTag: {
            LOG(DEBUG) << __func__ << " noop for vendor tag now";
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Parameter:IdNotSupported");
        }
    }
    LOG(ERROR) << " unsupported tag: " << toString(tag);
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "Parameter:IdNotSupported");
}

ndk::ScopedAStatus EqualizerSw::getState(State* _aidl_return) {
    *_aidl_return = mState;
    return ndk::ScopedAStatus::ok();
}

/// Private methods.
ndk::ScopedAStatus EqualizerSw::setCommonParameter(const Parameter::Common& common) {
    mCommonParam = common;
    LOG(DEBUG) << __func__ << " set: " << mCommonParam.toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::setSpecificParameter(const Parameter::Specific& specific) {
    if (Parameter::Specific::equalizer != specific.getTag()) {
        LOG(ERROR) << " unsupported effect: " << specific.toString();
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "EffectNotSupported");
    }

    auto& eqParam = specific.get<Parameter::Specific::equalizer>();
    auto tag = eqParam.getTag();
    switch (tag) {
        case Equalizer::bandLevels: {
            auto& bandLevels = eqParam.get<Equalizer::bandLevels>();
            const auto& [minItem, maxItem] = std::minmax_element(
                    bandLevels.begin(), bandLevels.end(),
                    [](const auto& a, const auto& b) { return a.index < b.index; });
            if (bandLevels.size() >= NUM_OF_BANDS || minItem->index < 0 ||
                maxItem->index >= NUM_OF_BANDS) {
                LOG(ERROR) << " bandLevels " << bandLevels.size() << "minIndex " << minItem->index
                           << "maxIndex " << maxItem->index << " illegal ";
                return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                        "ExceedMaxBandNum");
            }
            mBandLevels = bandLevels;
            return ndk::ScopedAStatus::ok();
        }
        case Equalizer::preset: {
            int preset = eqParam.get<Equalizer::preset>();
            if (preset < 0 || preset >= NUM_OF_PRESETS) {
                LOG(ERROR) << " preset: " << preset << " invalid";
                return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                        "ExceedMaxBandNum");
            }
            mPreset = preset;
            LOG(DEBUG) << __func__ << " preset set to " << mPreset;
            return ndk::ScopedAStatus::ok();
        }
        case Equalizer::vendor: {
            LOG(DEBUG) << __func__ << " noop for vendor tag now";
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "EqualizerTagNotSupported");
        }
    }

    LOG(ERROR) << __func__ << " unsupported eq param tag: " << toString(tag);
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "ParamNotSupported");
}

ndk::ScopedAStatus EqualizerSw::getCommonParameter(Parameter::Tag tag, Parameter* parameter) {
    switch (tag) {
        case Parameter::common: {
            parameter->set<Parameter::common>(mCommonParam);
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::device: {
            parameter->set<Parameter::device>(mDevice);
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::mode: {
            parameter->set<Parameter::mode>(mMode);
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::source: {
            parameter->set<Parameter::source>(mSource);
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::volume: {
            parameter->set<Parameter::volume>(mVolume);
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommonTagNotSupported");
        }
    }
}

ndk::ScopedAStatus EqualizerSw::getSpecificParameter(const Equalizer::Id& id,
                                                     Parameter::Specific* specific) {
    Equalizer eqParam;
    auto eqTag = id.get<Equalizer::Id::tag>();
    switch (eqTag) {
        case Equalizer::bandLevels: {
            eqParam.set<Equalizer::bandLevels>(mBandLevels);
            specific->set<Parameter::Specific::equalizer>(eqParam);
            return ndk::ScopedAStatus::ok();
        }
        case Equalizer::preset: {
            eqParam.set<Equalizer::preset>(mPreset);
            LOG(DEBUG) << __func__ << " preset " << mPreset;
            specific->set<Parameter::Specific::equalizer>(eqParam);
            return ndk::ScopedAStatus::ok();
        }
        case Equalizer::vendor: {
            LOG(DEBUG) << __func__ << " noop for vendor tag now";
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(eqTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "EqTagNotSupported");
        }
    }
    LOG(ERROR) << __func__ << " unsupported eq param: " << toString(eqTag);
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "ParamNotSupported");
}

void EqualizerSw::cleanUp() {
    if (State::PROCESSING == mState) {
        command(CommandId::STOP);
    }
    if (State::INIT != mState) {
        close();
    }
}

IEffect::Status EqualizerSw::status(binder_status_t status, size_t consumed, size_t produced) {
    IEffect::Status ret;
    ret.status = status;
    ret.fmqConsumed = consumed;
    ret.fmqProduced = produced;
    return ret;
}

// Processing method running in EffectWorker thread.
IEffect::Status EqualizerSw::effectProcessImpl(float* in, float* out, int frameCount) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " count " << frameCount;
    return status(STATUS_OK, mContext->availableToRead(), mContext->availableToWrite());
}

}  // namespace aidl::android::hardware::audio::effect
