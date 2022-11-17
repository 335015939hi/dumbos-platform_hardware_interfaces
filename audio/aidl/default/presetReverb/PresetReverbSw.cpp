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
#define LOG_TAG "AHAL_PresetReverbSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "PresetReverbSw.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kPresetReverbSwImplUUID;
using aidl::android::hardware::audio::effect::PresetReverbSw;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kPresetReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<PresetReverbSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t destroyEffect(const std::shared_ptr<IEffect>& instanceSp) {
    if (!instanceSp) {
        return EX_NONE;
    }
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

ndk::ScopedAStatus PresetReverbSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresetReverbSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::reverb != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& prParam = specific.get<Parameter::Specific::reverb>();
    auto tag = prParam.getTag();

    switch (tag) {
        case Reverb::roomLevelMb: {
            RETURN_IF(mContext->setPrRoomLevel(prParam.get<Reverb::roomLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::roomHfLevelMb: {
            RETURN_IF(mContext->setPrRoomHfLevel(prParam.get<Reverb::roomHfLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomHfLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayTimeMs: {
            RETURN_IF(mContext->setPrDecayTime(prParam.get<Reverb::decayTimeMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayTimeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayHfRatioPm: {
            RETURN_IF(mContext->setPrDecayHfRatio(prParam.get<Reverb::decayHfRatioPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayHfRatioFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::levelMb: {
            RETURN_IF(mContext->setPrLevel(prParam.get<Reverb::levelMb>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::delayMs: {
            RETURN_IF(mContext->setPrDelay(prParam.get<Reverb::delayMs>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::diffusionPm: {
            RETURN_IF(mContext->setPrDiffusion(prParam.get<Reverb::diffusionPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDiffusionFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::densityPm: {
            RETURN_IF(mContext->setPrDensity(prParam.get<Reverb::densityPm>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDensityFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::bypass: {
            RETURN_IF(mContext->setPrBypass(prParam.get<Reverb::bypass>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBypassFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
        }
    }
}

ndk::ScopedAStatus PresetReverbSw::getParameterSpecific(const Parameter::Id& id,
                                                        Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::reverbTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto prId = id.get<Parameter::Id::reverbTag>();
    auto prIdTag = prId.getTag();
    switch (prIdTag) {
        case Reverb::Id::commonTag:
            return getParameterReverb(prId.get<Reverb::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(prIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
    }
}

ndk::ScopedAStatus PresetReverbSw::getParameterReverb(const Reverb::Tag& tag,
                                                      Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    Reverb prParam;
    switch (tag) {
        case Reverb::roomLevelMb: {
            prParam.set<Reverb::roomLevelMb>(mContext->getPrRoomLevel());
            break;
        }
        case Reverb::roomHfLevelMb: {
            prParam.set<Reverb::roomHfLevelMb>(mContext->getPrRoomHfLevel());
            break;
        }
        case Reverb::decayTimeMs: {
            prParam.set<Reverb::decayTimeMs>(mContext->getPrDecayTime());
            break;
        }
        case Reverb::decayHfRatioPm: {
            prParam.set<Reverb::decayHfRatioPm>(mContext->getPrDecayHfRatio());
            break;
        }
        case Reverb::levelMb: {
            prParam.set<Reverb::levelMb>(mContext->getPrLevel());
            break;
        }
        case Reverb::delayMs: {
            prParam.set<Reverb::delayMs>(mContext->getPrDelay());
            break;
        }
        case Reverb::diffusionPm: {
            prParam.set<Reverb::diffusionPm>(mContext->getPrDiffusion());
            break;
        }
        case Reverb::densityPm: {
            prParam.set<Reverb::densityPm>(mContext->getPrDensity());
            break;
        }
        case Reverb::bypass: {
            prParam.set<Reverb::bypass>(mContext->getPrBypass());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::reverb>(prParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> PresetReverbSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<PresetReverbSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> PresetReverbSw::getContext() {
    return mContext;
}

RetCode PresetReverbSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status PresetReverbSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

}  // namespace aidl::android::hardware::audio::effect
