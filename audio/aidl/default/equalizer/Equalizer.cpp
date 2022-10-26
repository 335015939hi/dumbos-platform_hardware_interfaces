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

#include <cstddef>
#define LOG_TAG "AHAL_EqualizerSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "equalizer-impl/EqualizerSw.h"

using aidl::android::hardware::audio::effect::EqualizerSw;
using aidl::android::hardware::audio::effect::EqualizerSwImplUUID;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != EqualizerSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
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

namespace aidl::android::hardware::audio::effect {

ndk::ScopedAStatus EqualizerSw::open(const Parameter::Common& common,
                                     const Parameter::Specific& specific,
                                     OpenEffectReturn* _aidl_return) {
    LOG(DEBUG) << __func__;
    if (mState != State::INIT) {
        LOG(WARNING) << __func__ << " eq already open";
        return ndk::ScopedAStatus::ok();
    }

    LOG(DEBUG) << " common: " << common.toString() << " specific " << specific.toString();
    setSpecificParameter(specific);

    mContext = std::make_shared<EqualizerSwContext>(1 /* statusFmqDepth */, common);
    RETURN_IF(!mContext, EX_ILLEGAL_ARGUMENT, "createContextFailed");
    setCommonParameter(common);

    setContext(mContext);
    // create the worker thread
    if (RetCode::SUCCESS != createThread(LOG_TAG)) {
        LOG(ERROR) << __func__ << " created worker thread failed";
        mContext.reset();
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                                "FailedToCreateWorker");
    }
    mContext->dupeFmq(_aidl_return);
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
    LOG(DEBUG) << __func__ << kDesc.toString();
    *_aidl_return = kDesc;
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
            mContext->resetBuffer();
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
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");
    RETURN_IF(mContext == nullptr, EX_NULL_POINTER, "nullContext");
    LOG(DEBUG) << __func__ << " with: " << in_param.toString();

    auto tag = in_param.getTag();
    switch (tag) {
        case Parameter::common: {
            return setCommonParameter(in_param.get<Parameter::common>());
        }
        case Parameter::specific: {
            return setSpecificParameter(in_param.get<Parameter::specific>());
        }
        case Parameter::deviceDescription: {
            RETURN_IF(RetCode::SUCCESS != mContext->setOutputDevice(
                                                  in_param.get<Parameter::deviceDescription>()),
                      EX_ILLEGAL_ARGUMENT, "");
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::mode: {
            RETURN_IF(RetCode::SUCCESS != mContext->setAudioMode(in_param.get<Parameter::mode>()),
                      EX_ILLEGAL_ARGUMENT, "");
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::source: {
            RETURN_IF(
                    RetCode::SUCCESS != mContext->setAudioSource(in_param.get<Parameter::source>()),
                    EX_ILLEGAL_ARGUMENT, "");
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::volumeStereo: {
            RETURN_IF(RetCode::SUCCESS !=
                              mContext->setVolumeStereo(in_param.get<Parameter::volumeStereo>()),
                      EX_ILLEGAL_ARGUMENT, "");
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
    RETURN_IF(mContext == nullptr, EX_NULL_POINTER, "nullContext");
    LOG(DEBUG) << __func__ << in_paramId.toString();
    auto tag = in_paramId.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            return getCommonParameter(in_paramId.get<Parameter::Id::commonTag>(), _aidl_return);
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
    RETURN_IF(RetCode::SUCCESS != mContext->setParameterCommon(common), EX_ILLEGAL_ARGUMENT, "");
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
            parameter->set<Parameter::common>(mContext->getParameterCommon());
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::deviceDescription: {
            parameter->set<Parameter::deviceDescription>(mContext->getOutputDevice());
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::mode: {
            parameter->set<Parameter::mode>(mContext->getAudioMode());
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::source: {
            parameter->set<Parameter::source>(mContext->getAudioSource());
            return ndk::ScopedAStatus::ok();
        }
        case Parameter::volumeStereo: {
            parameter->set<Parameter::volumeStereo>(mContext->getVolumeStereo());
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
    auto eqTag = id.get<Equalizer::Id::commonTag>();
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
IEffect::Status EqualizerSw::effectProcessImpl(float* in, float* out, int process) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " process " << process;
    for (int i = 0; i < process; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, process, process};
}

}  // namespace aidl::android::hardware::audio::effect
