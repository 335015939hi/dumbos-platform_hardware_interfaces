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
#define LOG_TAG "AHAL_ReverbSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "ReverbSw.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::ReverbSw;
using aidl::android::hardware::audio::effect::ReverbSwImplUUID;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != ReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<ReverbSw>();
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

ndk::ScopedAStatus ReverbSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ReverbSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::reverb != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& rvParam = specific.get<Parameter::Specific::reverb>();
    auto tag = rvParam.getTag();

    switch (tag) {
        case Reverb::roomLevelMb: {
            RETURN_IF(mContext->setRvRoomLevel(rvParam.get<Reverb::roomLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::roomHfLevelMb: {
            RETURN_IF(mContext->setRvRoomHfLevel(rvParam.get<Reverb::roomHfLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomHfLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayTimeMs: {
            RETURN_IF(mContext->setRvDecayTime(rvParam.get<Reverb::decayTimeMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayTimeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::decayHfRatioPm: {
            RETURN_IF(mContext->setRvDecayHfRatio(rvParam.get<Reverb::decayHfRatioPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayHfRatioFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::levelMb: {
            RETURN_IF(mContext->setRvLevel(rvParam.get<Reverb::levelMb>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::delayMs: {
            RETURN_IF(mContext->setRvDelay(rvParam.get<Reverb::delayMs>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::diffusionPm: {
            RETURN_IF(mContext->setRvDiffusion(rvParam.get<Reverb::diffusionPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDiffusionFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::densityPm: {
            RETURN_IF(mContext->setRvDensity(rvParam.get<Reverb::densityPm>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDensityFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Reverb::bypass: {
            RETURN_IF(mContext->setRvBypass(rvParam.get<Reverb::bypass>()) != RetCode::SUCCESS,
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

ndk::ScopedAStatus ReverbSw::getParameterSpecific(const Parameter::Id& id,
                                                  Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::reverbTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto rvId = id.get<Parameter::Id::reverbTag>();
    auto rvIdTag = rvId.getTag();
    switch (rvIdTag) {
        case Reverb::Id::commonTag:
            return getParameterReverb(rvId.get<Reverb::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(rvIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
    }
}

ndk::ScopedAStatus ReverbSw::getParameterReverb(const Reverb::Tag& tag,
                                                Parameter::Specific* specific) {
    std::lock_guard lg(mMutex);
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    Reverb rvParam;
    switch (tag) {
        case Reverb::roomLevelMb: {
            rvParam.set<Reverb::roomLevelMb>(mContext->getRvRoomLevel());
            break;
        }
        case Reverb::roomHfLevelMb: {
            rvParam.set<Reverb::roomHfLevelMb>(mContext->getRvRoomHfLevel());
            break;
        }
        case Reverb::decayTimeMs: {
            rvParam.set<Reverb::decayTimeMs>(mContext->getRvDecayTime());
            break;
        }
        case Reverb::decayHfRatioPm: {
            rvParam.set<Reverb::decayHfRatioPm>(mContext->getRvDecayHfRatio());
            break;
        }
        case Reverb::levelMb: {
            rvParam.set<Reverb::levelMb>(mContext->getRvLevel());
            break;
        }
        case Reverb::delayMs: {
            rvParam.set<Reverb::delayMs>(mContext->getRvDelay());
            break;
        }
        case Reverb::diffusionPm: {
            rvParam.set<Reverb::diffusionPm>(mContext->getRvDiffusion());
            break;
        }
        case Reverb::densityPm: {
            rvParam.set<Reverb::densityPm>(mContext->getRvDensity());
            break;
        }
        case Reverb::bypass: {
            rvParam.set<Reverb::bypass>(mContext->getRvBypass());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::reverb>(rvParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> ReverbSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
        return mContext;
    }
    mContext = std::make_shared<ReverbSwContext>(1 /* statusFmqDepth */, common);
    return mContext;
}

RetCode ReverbSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status ReverbSw::effectProcessImpl(float* in, float* out, int process) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " process " << process;
    for (int i = 0; i < process; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, process, process};
}

}  // namespace aidl::android::hardware::audio::effect
