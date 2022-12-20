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
#define LOG_TAG "AHAL_DynamicsProcessingSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "DynamicsProcessingSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessingSw;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kDynamicsProcessingSwImplUUID;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kDynamicsProcessingSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<DynamicsProcessingSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != kDynamicsProcessingSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = DynamicsProcessingSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string DynamicsProcessingSw::kEffectName = "DynamicsProcessingSw";
const DynamicsProcessing::Capability DynamicsProcessingSw::kCapability = {
        .maxChannelCount = 3, .minCutOffFreq = 220.f, .maxCutOffFreq = 20000.f};
const Descriptor DynamicsProcessingSw::kDescriptor = {
        .common = {.id = {.type = kDynamicsProcessingTypeUUID,
                          .uuid = kDynamicsProcessingSwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = DynamicsProcessingSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = Capability::make<Capability::dynamicsProcessing>(
                DynamicsProcessingSw::kCapability)};

ndk::ScopedAStatus DynamicsProcessingSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DynamicsProcessingSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::dynamicsProcessing != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& dpParam = specific.get<Parameter::Specific::dynamicsProcessing>();
    auto tag = dpParam.getTag();
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            RETURN_IF(mContext->setEngineArchitecture(
                              dpParam.get<DynamicsProcessing::engineArchitecture>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setEngineArchitectureFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::preEq: {
            RETURN_IF(mContext->setPreEqChannelCfgs(dpParam.get<DynamicsProcessing::preEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEq: {
            RETURN_IF(mContext->setPostEqChannelCfgs(dpParam.get<DynamicsProcessing::postEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbc: {
            RETURN_IF(mContext->setMbcChannelCfgs(dpParam.get<DynamicsProcessing::mbc>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcChannelCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::preEqBand: {
            RETURN_IF(mContext->setPreEqBandCfgs(dpParam.get<DynamicsProcessing::preEqBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEqBand: {
            RETURN_IF(mContext->setPostEqBandCfgs(dpParam.get<DynamicsProcessing::postEqBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbcBand: {
            RETURN_IF(mContext->setMbcBandCfgs(dpParam.get<DynamicsProcessing::mbcBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcBandCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::limiter: {
            RETURN_IF(mContext->setLimiterCfgs(dpParam.get<DynamicsProcessing::limiter>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "limiterCfgsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::inputGainDb: {
            RETURN_IF(mContext->setInputGaindB(dpParam.get<DynamicsProcessing::inputGainDb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "inputGainDbCfgFailed");
            return ndk::ScopedAStatus::ok();
        }
        default:
            break;
    }

    LOG(ERROR) << __func__ << " unsupported dp param tag: " << toString(tag);
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "DynamicsProcessingTagNotSupported");
}

ndk::ScopedAStatus DynamicsProcessingSw::getParameterSpecific(const Parameter::Id& id,
                                                              Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::dynamicsProcessingTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto dpId = id.get<Parameter::Id::dynamicsProcessingTag>();
    auto dpIdTag = dpId.getTag();
    switch (dpIdTag) {
        case DynamicsProcessing::Id::commonTag:
            return getParameterDynamicsProcessing(dpId.get<DynamicsProcessing::Id::commonTag>(),
                                                  specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(dpIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "DynamicsProcessingTagNotSupported");
    }
}

ndk::ScopedAStatus DynamicsProcessingSw::getParameterDynamicsProcessing(
        const DynamicsProcessing::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    DynamicsProcessing dpParam;
    switch (tag) {
        case DynamicsProcessing::Tag::engineArchitecture: {
            dpParam.set<DynamicsProcessing::engineArchitecture>(mContext->getEngineArchitecture());
            break;
        }
        case DynamicsProcessing::Tag::preEq: {
            dpParam.set<DynamicsProcessing::preEq>(mContext->getPreEqChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::postEq: {
            dpParam.set<DynamicsProcessing::postEq>(mContext->getPostEqChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::mbc: {
            dpParam.set<DynamicsProcessing::mbc>(mContext->getMbcChannelCfgs());
            break;
        }
        case DynamicsProcessing::Tag::preEqBand: {
            dpParam.set<DynamicsProcessing::preEqBand>(mContext->getPreEqBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::postEqBand: {
            dpParam.set<DynamicsProcessing::postEqBand>(mContext->getPostEqBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::mbcBand: {
            dpParam.set<DynamicsProcessing::mbcBand>(mContext->getMbcBandCfgs());
            break;
        }
        case DynamicsProcessing::Tag::limiter: {
            dpParam.set<DynamicsProcessing::limiter>(mContext->getLimiterCfgs());
            break;
        }
        case DynamicsProcessing::Tag::inputGainDb: {
            dpParam.set<DynamicsProcessing::inputGainDb>(mContext->getInputGaindB());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "DynamicsProcessingTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::dynamicsProcessing>(dpParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> DynamicsProcessingSw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        int channelsInEffectArch =
                ::android::hardware::audio::common::getChannelCount(common.output.base.channelMask);
        if (channelsInEffectArch <= DynamicsProcessingSw::kCapability.maxChannelCount) {
            mContext = std::make_shared<DynamicsProcessingSwContext>(1, common);
        }
    }
    return mContext;
}

std::shared_ptr<EffectContext> DynamicsProcessingSw::getContext() {
    return mContext;
}

RetCode DynamicsProcessingSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status DynamicsProcessingSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode DynamicsProcessingSwContext::setEngineArchitecture(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    if (cfg.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION &&
        cfg.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION) {
        LOG(ERROR) << __func__
                   << " invalid Resolution Preference: " << (int)cfg.resolutionPreference;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (cfg.preferredFrameDurationMs < 0) {
        LOG(ERROR) << __func__ << " invalid frame duration: " << cfg.preferredFrameDurationMs;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (cfg.preEqBand.inUse && cfg.preEqBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid pre eq band count: " << cfg.preEqBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (cfg.postEqBand.inUse && cfg.postEqBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid post eq band count: " << cfg.postEqBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (cfg.mbcBand.inUse && cfg.mbcBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid mbc band count: " << cfg.mbcBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    mEngineSettings = cfg;
    if (cfg.preEqBand.inUse) {
        mPreEqChBands.resize(mChannelCount * cfg.preEqBand.bandCount);
    }
    if (cfg.postEqBand.inUse) {
        mPostEqChBands.resize(mChannelCount * cfg.postEqBand.bandCount);
    }
    if (cfg.mbcBand.inUse) {
        mMbcChBands.resize(mChannelCount * cfg.mbcBand.bandCount);
    }
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setChannelCfgs(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs,
        std::vector<DynamicsProcessing::BandChannelConfig>& targetCfgs,
        const DynamicsProcessing::BandEnablement& stage) {
    // in engine architecture, if stage is disabled, verify if cfgs tell otherwise
    for (auto& cfg : cfgs) {
        RETURN_VALUE_IF((cfg.enablement.inUse != stage.inUse), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid inUse flag");
    }
    // if stage is not in use, no need to set anything
    if (!stage.inUse) return RetCode::SUCCESS;

    std::vector<bool> filled(mChannelCount, false);
    for (auto& cfg : cfgs) {
        // verify and set
        RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
        RETURN_VALUE_IF((cfg.enablement.bandCount != stage.bandCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid bandCount");
        targetCfgs[cfg.channel] = cfg;
        filled[cfg.channel] = true;
    }
    for (int i = 0; i < mChannelCount; i++) {
        if (!filled[i]) {
            // last config to duplicate at all missing cfgs
            DynamicsProcessing::BandChannelConfig last = cfgs[cfgs.size() - 1];
            last.channel = i;
            targetCfgs[i] = last;
        }
    }
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPreEqChannelCfgs(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    DynamicsProcessing::BandEnablement stage{mEngineSettings.preEqBand.inUse,
                                             mEngineSettings.preEqBand.bandCount};
    return setChannelCfgs(cfgs, mPreEqChCfgs, stage);
}

RetCode DynamicsProcessingSwContext::setPostEqChannelCfgs(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    DynamicsProcessing::BandEnablement stage{mEngineSettings.postEqBand.inUse,
                                             mEngineSettings.postEqBand.bandCount};
    return setChannelCfgs(cfgs, mPostEqChCfgs, stage);
}

RetCode DynamicsProcessingSwContext::setMbcChannelCfgs(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    DynamicsProcessing::BandEnablement stage{mEngineSettings.mbcBand.inUse,
                                             mEngineSettings.mbcBand.bandCount};
    return setChannelCfgs(cfgs, mMbcChCfgs, stage);
}

RetCode DynamicsProcessingSwContext::setEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
        std::vector<DynamicsProcessing::EqBandConfig>& targetCfgs,
        const DynamicsProcessing::BandEnablement& stage) {
    // if stage is not in use, no need to set anything
    if (!stage.inUse) return RetCode::SUCCESS;

    std::vector<bool> filled(mChannelCount * stage.bandCount, false);
    for (auto& cfg : cfgs) {
        // verify and set
        RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
        RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= stage.bandCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid band index");
        RETURN_VALUE_IF((cfg.cutoffFrequency < DynamicsProcessingSw::kCapability.minCutOffFreq ||
                         cfg.cutoffFrequency > DynamicsProcessingSw::kCapability.maxCutOffFreq),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid band cut off frequency");
        targetCfgs[cfg.channel * stage.bandCount + cfg.band] = cfg;
        filled[cfg.channel * stage.bandCount + cfg.band] = true;
    }
    for (int i = 0; i < mChannelCount; i++) {
        int bandsReceived = 0;
        for (int j = 0; j < stage.bandCount; j++) {
            if (filled[i * stage.bandCount + j]) bandsReceived++;
        }
        RETURN_VALUE_IF((bandsReceived != 0 && bandsReceived != stage.bandCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "incomplete channel band config");
        if (bandsReceived == stage.bandCount) {
            for (int j = 1; j < stage.bandCount; j++) {
                RETURN_VALUE_IF((targetCfgs[i * stage.bandCount + j].cutoffFrequency <=
                                 targetCfgs[i * stage.bandCount + j - 1].cutoffFrequency),
                                RetCode::ERROR_ILLEGAL_PARAMETER, "invalid band cut off frequency");
            }
        } else {
            // for now assume channel id 0 is always configured
            RETURN_VALUE_IF(i == 0, RetCode::ERROR_ILLEGAL_PARAMETER,
                            "configure channel index 0 for copying");
            for (int j = 0; j < stage.bandCount; j++) {
                targetCfgs[i * stage.bandCount + j] = targetCfgs[(i - 1) * stage.bandCount + j];
                targetCfgs[i * stage.bandCount + j].channel = i;
            }
        }
    }
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPreEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing::BandEnablement stage{mEngineSettings.preEqBand.inUse,
                                             mEngineSettings.preEqBand.bandCount};
    return setEqBandCfgs(cfgs, mPreEqChBands, stage);
}

RetCode DynamicsProcessingSwContext::setPostEqBandCfgs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing::BandEnablement stage{mEngineSettings.postEqBand.inUse,
                                             mEngineSettings.postEqBand.bandCount};
    return setEqBandCfgs(cfgs, mPostEqChBands, stage);
}

RetCode DynamicsProcessingSwContext::setMbcBandCfgs(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    // if stage is not in use, no need to set anything
    if (!mEngineSettings.mbcBand.inUse) return RetCode::SUCCESS;

    int bandCount = mEngineSettings.mbcBand.bandCount;
    std::vector<bool> filled(mChannelCount * bandCount, false);
    for (auto& cfg : cfgs) {
        // verify and set
        RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
        RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= bandCount), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid band index");
        RETURN_VALUE_IF((cfg.cutoffFrequencyHz < DynamicsProcessingSw::kCapability.minCutOffFreq ||
                         cfg.cutoffFrequencyHz > DynamicsProcessingSw::kCapability.maxCutOffFreq),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid band cut off frequency");
        RETURN_VALUE_IF((cfg.attackTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid attack time");
        RETURN_VALUE_IF((cfg.releaseTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid release time");
        RETURN_VALUE_IF((cfg.ratio < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid compressor ratio");
        RETURN_VALUE_IF((cfg.thresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid dBFS threshold");
        RETURN_VALUE_IF((cfg.kneeWidthDb < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid knee width");
        RETURN_VALUE_IF((cfg.noiseGateThresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid dBFS noise gate threshold");
        RETURN_VALUE_IF((cfg.expanderRatio < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid expander ratio");
        mMbcChBands[cfg.channel * bandCount + cfg.band] = cfg;
        filled[cfg.channel * bandCount + cfg.band] = true;
    }
    for (int i = 0; i < mChannelCount; i++) {
        int bandsReceived = 0;
        for (int j = 0; j < bandCount; j++) {
            if (filled[i * bandCount + j]) bandsReceived++;
        }
        RETURN_VALUE_IF((bandsReceived != 0 && bandsReceived != bandCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "incomplete channel band config");
        if (bandsReceived == bandCount) {
            for (int j = 1; j < bandCount; j++) {
                RETURN_VALUE_IF((mMbcChBands[i * bandCount + j].cutoffFrequencyHz <=
                                 mMbcChBands[i * bandCount + j - 1].cutoffFrequencyHz),
                                RetCode::ERROR_ILLEGAL_PARAMETER, "invalid band cut off frequency");
            }
        } else {
            // for now assume channel id 0 is always configured
            RETURN_VALUE_IF(i == 0, RetCode::ERROR_ILLEGAL_PARAMETER,
                            "configure channel index 0 for copying");
            for (int j = 0; j < bandCount; j++) {
                mMbcChBands[i * bandCount + j] = mMbcChBands[(i - 1) * bandCount + j];
                mMbcChBands[i * bandCount + j].channel = i;
            }
        }
    }
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setLimiterCfgs(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    // in engine architecture, if pre eq is disabled, verify if cfgs tell otherwise
    for (auto& cfg : cfgs) {
        RETURN_VALUE_IF((cfg.inUse != mEngineSettings.limiterInUse),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid limiter in use flag");
    }
    // if stage is not in use, no need to set anything
    if (!mEngineSettings.limiterInUse) return RetCode::SUCCESS;

    std::vector<bool> filled(mChannelCount, false);
    for (auto& cfg : cfgs) {
        // verify and set
        RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                        RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
        RETURN_VALUE_IF((cfg.attackTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid attack time");
        RETURN_VALUE_IF((cfg.releaseTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid release time");
        RETURN_VALUE_IF((cfg.ratio < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid compressor ratio");
        RETURN_VALUE_IF((cfg.thresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                        "invalid dBFS threshold");
        mLimiterCfgs[cfg.channel] = cfg;
        filled[cfg.channel] = true;
    }
    for (int i = 0; i < mChannelCount; i++) {
        if (!filled[i]) {
            // last config to duplicate at all missing cfgs
            DynamicsProcessing::LimiterConfig last = cfgs[cfgs.size() - 1];
            last.channel = i;
            mLimiterCfgs[i] = last;
        }
    }
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
