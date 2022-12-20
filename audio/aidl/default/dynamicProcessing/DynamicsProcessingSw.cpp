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
const DynamicsProcessing::Capability DynamicsProcessingSw::kCapability;
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
            RETURN_IF(mContext->setPreEqChannelConfig(dpParam.get<DynamicsProcessing::preEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqChannelConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEq: {
            RETURN_IF(mContext->setPostEqChannelConfig(dpParam.get<DynamicsProcessing::postEq>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqChannelConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbc: {
            RETURN_IF(mContext->setMbcChannelConfig(dpParam.get<DynamicsProcessing::mbc>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcChannelConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::preEqBand: {
            RETURN_IF(mContext->setPreEqBandConfig(dpParam.get<DynamicsProcessing::preEqBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPreEqBandConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::postEqBand: {
            RETURN_IF(mContext->setPostEqBandConfig(
                              dpParam.get<DynamicsProcessing::postEqBand>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPostEqBandConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::mbcBand: {
            RETURN_IF(mContext->setMbcBandConfig(dpParam.get<DynamicsProcessing::mbcBand>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMbcBandConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::limiter: {
            RETURN_IF(mContext->setLimiterConfig(dpParam.get<DynamicsProcessing::limiter>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "limiterConfigFailed");
            return ndk::ScopedAStatus::ok();
        }
        case DynamicsProcessing::inputGainDb: {
            RETURN_IF(mContext->setInputGaindB(dpParam.get<DynamicsProcessing::inputGainDb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "inputGainDbConfigFailed");
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
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::preEq>();
            RETURN_IF(mContext->getPreEqChannelConfig(cfg) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "getPreEqChannelConfigFailed");
            dpParam.set<DynamicsProcessing::preEq>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::postEq: {
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::postEq>();
            RETURN_IF(mContext->getPostEqChannelConfig(cfg) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "getPostEqChannelConfigFailed");
            dpParam.set<DynamicsProcessing::postEq>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::mbc: {
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::mbc>();
            RETURN_IF(mContext->getMbcChannelConfig(cfg) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "getMbcChannelConfigFailed");
            dpParam.set<DynamicsProcessing::mbc>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::preEqBand: {
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::preEqBand>();
            RETURN_IF(mContext->getPreEqBandConfig(cfg) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "getPreEqBandConfigFailed");
            dpParam.set<DynamicsProcessing::preEqBand>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::postEqBand: {
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::postEqBand>();
            RETURN_IF(mContext->getPostEqBandConfig(cfg) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "getPostEqBandConfigFailed");
            dpParam.set<DynamicsProcessing::postEqBand>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::mbcBand: {
            auto& dpInParam = specific->get<Parameter::Specific::dynamicsProcessing>();
            auto& cfg = dpInParam.get<DynamicsProcessing::mbcBand>();
            RETURN_IF(mContext->getMbcBandConfig(cfg) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "getMbcBandConfigFailed");
            dpParam.set<DynamicsProcessing::mbcBand>(cfg);
            break;
        }
        case DynamicsProcessing::Tag::limiter: {
            dpParam.set<DynamicsProcessing::limiter>(mContext->getLimiterConfig());
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
        mContext = std::make_shared<DynamicsProcessingSwContext>(1 /* statusFmqDepth */, common);
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
        const DynamicsProcessing::EngineArchitecture& config) {
    if (config.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION &&
        config.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION) {
        LOG(ERROR) << __func__
                   << " invalid Resolution Preference: " << (int)config.resolutionPreference;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (config.preferredFrameDurationMs < 0) {
        LOG(ERROR) << __func__ << " invalid frame duration: " << config.preferredFrameDurationMs;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (config.preEqBand.inUse && config.preEqBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid pre eq band count: " << config.preEqBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (config.postEqBand.inUse && config.postEqBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid post eq band count: " << config.postEqBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    if (config.mbcBand.inUse && config.mbcBand.bandCount <= 0) {
        LOG(ERROR) << __func__ << " invalid mbc band count: " << config.mbcBand.bandCount;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    mEngineSettings = config;
    if (config.preEqBand.inUse) {
        for (size_t i = 0; i < mPreEqChBands.size(); i++) {
            mPreEqChBands[i].resize(config.preEqBand.bandCount);
        }
    }
    if (config.postEqBand.inUse) {
        for (size_t i = 0; i < mPostEqChBands.size(); i++) {
            mPostEqChBands[i].resize(config.postEqBand.bandCount);
        }
    }
    if (config.mbcBand.inUse) {
        for (size_t i = 0; i < mMbcChBands.size(); i++) {
            mMbcChBands[i].resize(config.mbcBand.bandCount);
        }
    }
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPreEqChannelConfig(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    RETURN_VALUE_IF((cfg.enablement.inUse != mEngineSettings.preEqBand.inUse),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq inUse flag");
    if (!mEngineSettings.preEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.enablement.bandCount != mEngineSettings.preEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq bandCount");
    mPreEqChCfgs[cfg.channel] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getPreEqChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    if (!mEngineSettings.preEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    cfg = mPreEqChCfgs[cfg.channel];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPostEqChannelConfig(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    RETURN_VALUE_IF((cfg.enablement.inUse != mEngineSettings.postEqBand.inUse),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid post eq inUse flag");
    if (!mEngineSettings.postEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.enablement.bandCount != mEngineSettings.postEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid post eq bandCount");
    mPostEqChCfgs[cfg.channel] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getPostEqChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    if (!mEngineSettings.postEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    cfg = mPostEqChCfgs[cfg.channel];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setMbcChannelConfig(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    RETURN_VALUE_IF((cfg.enablement.inUse != mEngineSettings.mbcBand.inUse),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid mbc in use flag");
    if (!mEngineSettings.mbcBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.enablement.bandCount != mEngineSettings.mbcBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid mbc bandCount");
    mMbcChCfgs[cfg.channel] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getMbcChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    if (!mEngineSettings.mbcBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    cfg = mMbcChCfgs[cfg.channel];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPreEqBandConfig(
        const DynamicsProcessing::EqBandConfig& cfg) {
    if (!mEngineSettings.preEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.preEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    RETURN_VALUE_IF(
            (cfg.cutoffFrequency < DEFAULT_MIN_FREQUENCY || cfg.band > DEFAULT_MAX_FREQUENCY),
            RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    if (cfg.band > 0) {
        RETURN_VALUE_IF(
                (cfg.cutoffFrequency <= mPreEqChBands[cfg.channel][cfg.band - 1].cutoffFrequency),
                RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    }
    mPreEqChBands[cfg.channel][cfg.band] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getPreEqBandConfig(DynamicsProcessing::EqBandConfig& cfg) {
    if (!mEngineSettings.preEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.preEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    cfg = mPreEqChBands[cfg.channel][cfg.band];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setPostEqBandConfig(
        const DynamicsProcessing::EqBandConfig& cfg) {
    if (!mEngineSettings.postEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.postEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    RETURN_VALUE_IF(
            (cfg.cutoffFrequency < DEFAULT_MIN_FREQUENCY || cfg.band > DEFAULT_MAX_FREQUENCY),
            RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    if (cfg.band > 0) {
        RETURN_VALUE_IF(
                (cfg.cutoffFrequency <= mPostEqChBands[cfg.channel][cfg.band - 1].cutoffFrequency),
                RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    }
    mPostEqChBands[cfg.channel][cfg.band] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getPostEqBandConfig(DynamicsProcessing::EqBandConfig& cfg) {
    if (!mEngineSettings.postEqBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.postEqBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    cfg = mPostEqChBands[cfg.channel][cfg.band];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setMbcBandConfig(
        const DynamicsProcessing::MbcBandConfig& cfg) {
    if (!mEngineSettings.mbcBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.mbcBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    RETURN_VALUE_IF(
            (cfg.cutoffFrequencyHz < DEFAULT_MIN_FREQUENCY || cfg.band > DEFAULT_MAX_FREQUENCY),
            RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    if (cfg.band > 0) {
        RETURN_VALUE_IF(
                (cfg.cutoffFrequencyHz <= mMbcChBands[cfg.channel][cfg.band - 1].cutoffFrequencyHz),
                RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band cut off frequency");
    }
    RETURN_VALUE_IF((cfg.attackTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid attack time");
    RETURN_VALUE_IF((cfg.releaseTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid release time");
    RETURN_VALUE_IF((cfg.ratio < 0), RetCode::ERROR_ILLEGAL_PARAMETER, "invalid compressor ratio");
    RETURN_VALUE_IF((cfg.thresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid dBFS threshold");
    RETURN_VALUE_IF((cfg.kneeWidthDb < 0), RetCode::ERROR_ILLEGAL_PARAMETER, "invalid knee width");
    RETURN_VALUE_IF((cfg.noiseGateThresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid dBFS noise gate threshold");
    RETURN_VALUE_IF((cfg.expanderRatio < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid expander ratio");
    mMbcChBands[cfg.channel][cfg.band] = cfg;
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::getMbcBandConfig(DynamicsProcessing::MbcBandConfig& cfg) {
    if (!mEngineSettings.mbcBand.inUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.band < 0 || cfg.band >= mEngineSettings.mbcBand.bandCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid pre eq band index");
    cfg = mMbcChBands[cfg.channel][cfg.band];
    return RetCode::SUCCESS;
}

RetCode DynamicsProcessingSwContext::setLimiterConfig(
        const DynamicsProcessing::LimiterConfig& cfg) {
    RETURN_VALUE_IF((cfg.inUse != mEngineSettings.limiterInUse), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid limiter in use flag");
    if (!mEngineSettings.limiterInUse) return RetCode::SUCCESS;
    RETURN_VALUE_IF((cfg.channel < 0 || cfg.channel >= mChannelCount),
                    RetCode::ERROR_ILLEGAL_PARAMETER, "invalid channel index");
    RETURN_VALUE_IF((cfg.attackTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid attack time");
    RETURN_VALUE_IF((cfg.releaseTimeMs < 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid release time");
    RETURN_VALUE_IF((cfg.ratio < 0), RetCode::ERROR_ILLEGAL_PARAMETER, "invalid compressor ratio");
    RETURN_VALUE_IF((cfg.thresholdDb > 0), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "invalid dBFS threshold");
    mLimiterCfg = cfg;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
