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

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class DynamicsProcessingSwContext final : public EffectContext {
  public:
    DynamicsProcessingSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
        mChannelCount =
                ::android::hardware::audio::common::getChannelCount(common.input.base.channelMask);
        mPreEqChCfgs.reserve(mChannelCount);
        mPostEqChCfgs.reserve(mChannelCount);
        mMbcChCfgs.reserve(mChannelCount);
        mPreEqChBands.reserve(mChannelCount);
        mPostEqChBands.reserve(mChannelCount);
        mMbcChBands.reserve(mChannelCount);
    }

    // set params
    RetCode setEngineArchitecture(const DynamicsProcessing::EngineArchitecture& config);
    RetCode setPreEqChannelConfig(const DynamicsProcessing::BandChannelConfig& cfg);
    RetCode setPostEqChannelConfig(const DynamicsProcessing::BandChannelConfig& cfg);
    RetCode setMbcChannelConfig(const DynamicsProcessing::BandChannelConfig& cfg);
    RetCode setPreEqBandConfig(const DynamicsProcessing::EqBandConfig& cfg);
    RetCode setPostEqBandConfig(const DynamicsProcessing::EqBandConfig& cfg);
    RetCode setMbcBandConfig(const DynamicsProcessing::MbcBandConfig& cfg);
    RetCode setLimiterConfig(const DynamicsProcessing::LimiterConfig& cfg);
    RetCode setInputGaindB(float inputGain) {
        mInputGaindB = inputGain;
        return RetCode::SUCCESS;
    }

    // get params
    DynamicsProcessing::EngineArchitecture getEngineArchitecture() { return mEngineSettings; }
    RetCode getPreEqChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    RetCode getPostEqChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    RetCode getMbcChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    RetCode getPreEqBandConfig(DynamicsProcessing::EqBandConfig& cfg);
    RetCode getPostEqBandConfig(DynamicsProcessing::EqBandConfig& cfg);
    RetCode getMbcBandConfig(DynamicsProcessing::MbcBandConfig& cfg);
    DynamicsProcessing::LimiterConfig getLimiterConfig() { return mLimiterCfg; }
    float getInputGaindB() { return mInputGaindB; }

  private:
    static constexpr float DEFAULT_MIN_FREQUENCY = 220;    // Hz
    static constexpr float DEFAULT_MAX_FREQUENCY = 20000;  // Hz

    int mChannelCount;
    DynamicsProcessing::EngineArchitecture mEngineSettings;
    std::vector<DynamicsProcessing::BandChannelConfig> mPreEqChCfgs;
    std::vector<DynamicsProcessing::BandChannelConfig> mPostEqChCfgs;
    std::vector<DynamicsProcessing::BandChannelConfig> mMbcChCfgs;
    std::vector<std::vector<DynamicsProcessing::EqBandConfig>> mPreEqChBands;
    std::vector<std::vector<DynamicsProcessing::EqBandConfig>> mPostEqChBands;
    std::vector<std::vector<DynamicsProcessing::MbcBandConfig>> mMbcChBands;
    float mInputGaindB = 0.f;
    DynamicsProcessing::LimiterConfig mLimiterCfg;
};

class DynamicsProcessingSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const DynamicsProcessing::Capability kCapability;
    static const Descriptor kDescriptor;
    DynamicsProcessingSw() { LOG(DEBUG) << __func__; }
    ~DynamicsProcessingSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    std::shared_ptr<EffectContext> getContext() override;
    RetCode releaseContext() override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; };

  private:
    std::shared_ptr<DynamicsProcessingSwContext> mContext;
    ndk::ScopedAStatus getParameterDynamicsProcessing(const DynamicsProcessing::Tag& tag,
                                                      Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
