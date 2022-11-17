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
#define LOG_TAG "AHAL_EnvReverbSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "EnvReverbSw.h"

using aidl::android::hardware::audio::effect::EnvReverbSw;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kEnvReverbSwImplUUID;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kEnvReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<EnvReverbSw>();
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

ndk::ScopedAStatus EnvReverbSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EnvReverbSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::reverb != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& erParam = specific.get<Parameter::Specific::reverb>();
    auto tag = erParam.getTag();

    switch (tag) {
        case Reverb::roomLevelMb: {
            RETURN_IF(mContext->setErRoomLevel(erParam.get<Reverb::roomLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::roomHfLevelMb: {
            RETURN_IF(mContext->setErRoomHfLevel(erParam.get<Reverb::roomHfLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomHfLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayTimeMs: {
            RETURN_IF(mContext->setErDecayTime(erParam.get<Reverb::decayTimeMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayTimeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayHfRatioPm: {
            RETURN_IF(mContext->setErDecayHfRatio(erParam.get<Reverb::decayHfRatioPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayHfRatioFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::levelMb: {
            RETURN_IF(mContext->setErLevel(erParam.get<Reverb::levelMb>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::delayMs: {
            RETURN_IF(mContext->setErDelay(erParam.get<Reverb::delayMs>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::diffusionPm: {
            RETURN_IF(mContext->setErDiffusion(erParam.get<Reverb::diffusionPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDiffusionFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::densityPm: {
            RETURN_IF(mContext->setErDensity(erParam.get<Reverb::densityPm>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDensityFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::bypass: {
            RETURN_IF(mContext->setErBypass(erParam.get<Reverb::bypass>()) != RetCode::SUCCESS,
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

ndk::ScopedAStatus EnvReverbSw::getParameterSpecific(const Parameter::Id& id,
                                                     Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::reverbTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto erId = id.get<Parameter::Id::reverbTag>();
    auto erIdTag = erId.getTag();
    switch (erIdTag) {
        case Reverb::Id::commonTag:
            return getParameterReverb(erId.get<Reverb::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(erIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
    }
}

ndk::ScopedAStatus EnvReverbSw::getParameterReverb(const Reverb::Tag& tag,
                                                   Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    Reverb erParam;
    switch (tag) {
        case Reverb::roomLevelMb: {
            erParam.set<Reverb::roomLevelMb>(mContext->getErRoomLevel());
            break;
        }
        case Reverb::roomHfLevelMb: {
            erParam.set<Reverb::roomHfLevelMb>(mContext->getErRoomHfLevel());
            break;
        }
        case Reverb::decayTimeMs: {
            erParam.set<Reverb::decayTimeMs>(mContext->getErDecayTime());
            break;
        }
        case Reverb::decayHfRatioPm: {
            erParam.set<Reverb::decayHfRatioPm>(mContext->getErDecayHfRatio());
            break;
        }
        case Reverb::levelMb: {
            erParam.set<Reverb::levelMb>(mContext->getErLevel());
            break;
        }
        case Reverb::delayMs: {
            erParam.set<Reverb::delayMs>(mContext->getErDelay());
            break;
        }
        case Reverb::diffusionPm: {
            erParam.set<Reverb::diffusionPm>(mContext->getErDiffusion());
            break;
        }
        case Reverb::densityPm: {
            erParam.set<Reverb::densityPm>(mContext->getErDensity());
            break;
        }
        case Reverb::bypass: {
            erParam.set<Reverb::bypass>(mContext->getErBypass());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::reverb>(erParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> EnvReverbSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<EnvReverbSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> EnvReverbSw::getContext() {
    return mContext;
}

RetCode EnvReverbSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status EnvReverbSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

}  // namespace aidl::android::hardware::audio::effect
