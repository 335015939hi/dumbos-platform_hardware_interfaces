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

#include <aidl/Vintf.h>

#define LOG_TAG "VtsHalDynamicsProcessingTest"

#include <Utils.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessing;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kDynamicsProcessingTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
class DynamicsProcessingTestHelper : public EffectHelper {
  public:
    DynamicsProcessingTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair,
                                 int32_t channelLayOut = AudioChannelLayout::LAYOUT_STEREO) {
        std::tie(mFactory, mDescriptor) = pair;
        mChannelLayout = channelLayOut;
        mChannelCount = ::android::hardware::audio::common::getChannelCount(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout));
    }

    // setup
    void SetUpDynamicsProcessingEffect() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                0x100 /* iFrameCount */, 0x100 /* oFrameCount */,
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout),
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout));
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    Parameter::Specific getDefaultParamSpecific() {
        DynamicsProcessing dp =
                DynamicsProcessing::make<DynamicsProcessing::engineArchitecture>(mEngineConfig);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::dynamicsProcessing>(dp);
        return specific;
    }

    // teardown
    void TearDownDynamicsProcessingEffect() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    // utils functions for parameter checking
    bool isParamValid(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dp,
                      const Descriptor& desc);
    bool isParamEqual(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dpRef,
                      const DynamicsProcessing& dpTest);
    bool isEngineConfigValid(const DynamicsProcessing::EngineArchitecture& cfg);
    bool isPreEqBandChannelConfigValid(
            const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs);
    bool isPostEqBandChannelConfigValid(
            const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs);
    bool isMbcBandChannelConfigValid(
            const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs);
    bool isEqBandConfigValid(const DynamicsProcessing::Capability& cap,
                             const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                             bool stageInUse, int bandCount);
    bool isMbcBandConfigValid(const DynamicsProcessing::Capability& cap,
                              const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                              bool stageInUse, int bandCount);
    bool isLimiterConfigValid(const std::vector<DynamicsProcessing::LimiterConfig>& cfgs);

    bool isEngineConfigEqual(const DynamicsProcessing::EngineArchitecture& refCfg,
                             const DynamicsProcessing::EngineArchitecture& testCfg);
    bool isBandChannelConfigEqual(const DynamicsProcessing::BandChannelConfig& refCfg,
                                  const DynamicsProcessing::BandChannelConfig& testCfg,
                                  bool stageInUse);
    bool isBandChannelConfigEqual(
            const std::vector<DynamicsProcessing::BandChannelConfig>& refCfgs,
            const std::vector<DynamicsProcessing::BandChannelConfig>& testCfgs, bool stageInUse);
    bool isEqBandConfigEqual(const DynamicsProcessing::EqBandConfig& refCfg,
                             const DynamicsProcessing::EqBandConfig& testCfg, bool stageInUse);
    bool isEqBandConfigEqual(const std::vector<DynamicsProcessing::EqBandConfig>& refCfgs,
                             const std::vector<DynamicsProcessing::EqBandConfig>& testCfgs,
                             bool stageInUse);
    bool isMbcBandConfigEqual(const DynamicsProcessing::MbcBandConfig& refCfg,
                              const DynamicsProcessing::MbcBandConfig& testCfg, bool stageInUse);
    bool isMbcBandConfigEqual(const std::vector<DynamicsProcessing::MbcBandConfig>& refCfgs,
                              const std::vector<DynamicsProcessing::MbcBandConfig>& testCfgs,
                              bool stageInUse);
    bool isLimiterConfigEqual(const DynamicsProcessing::LimiterConfig& refCfg,
                              const DynamicsProcessing::LimiterConfig& testCfg);
    bool isLimiterConfigEqual(const std::vector<DynamicsProcessing::LimiterConfig>& refCfgs,
                              const std::vector<DynamicsProcessing::LimiterConfig>& testCfgs);

    // get set params and validate
    void SetAndGetDynamicsProcessingParameters();

    // enqueue test parameters
    void addEngineConfig(DynamicsProcessing::EngineArchitecture& cfg);
    void addPreEqBandChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    void addPostEqBandChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    void addMbcBandChannelConfig(DynamicsProcessing::BandChannelConfig& cfg);
    void addPreEqBandConfigs(std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addPostEqBandConfigs(std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addMbcBandConfigs(std::vector<DynamicsProcessing::MbcBandConfig>& cfgs);
    void addLimiterConfig(DynamicsProcessing::LimiterConfig& cfg);
    void addInputGain(float inputGaindB);

    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    DynamicsProcessing::EngineArchitecture mEngineConfig{
            .resolutionPreference =
                    DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
            .preferredFrameDurationMs = 10.0f,
            .preEqBand = {.inUse = true, .bandCount = 5},
            .postEqBand = {.inUse = true, .bandCount = 5},
            .mbcBand = {.inUse = true, .bandCount = 5},
            .limiterInUse = true,
    };

  private:
    int32_t mChannelLayout;
    int mChannelCount;
    std::vector<std::pair<DynamicsProcessing::Tag, DynamicsProcessing>> mTags;
    void CleanUp() { mTags.clear(); }
};

bool DynamicsProcessingTestHelper::isParamValid(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dp,
                                                const Descriptor& desc) {
    const DynamicsProcessing::Capability& dpCap =
            desc.capability.get<Capability::dynamicsProcessing>();
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigValid(dp.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            return isPreEqBandChannelConfigValid(dp.get<DynamicsProcessing::preEq>());
        }
        case DynamicsProcessing::postEq: {
            return isPostEqBandChannelConfigValid(dp.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isMbcBandChannelConfigValid(dp.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isEqBandConfigValid(dpCap, dp.get<DynamicsProcessing::preEqBand>(),
                                       mEngineConfig.preEqBand.inUse,
                                       mEngineConfig.preEqBand.bandCount);
        }
        case DynamicsProcessing::postEqBand: {
            return isEqBandConfigValid(dpCap, dp.get<DynamicsProcessing::postEqBand>(),
                                       mEngineConfig.postEqBand.inUse,
                                       mEngineConfig.postEqBand.bandCount);
        }
        case DynamicsProcessing::mbcBand: {
            return isMbcBandConfigValid(dpCap, dp.get<DynamicsProcessing::mbcBand>(),
                                        mEngineConfig.mbcBand.inUse,
                                        mEngineConfig.mbcBand.bandCount);
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigValid(dp.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGainDb: {
            return true;
        }
        case DynamicsProcessing::vendorExtension: {
            return true;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isParamEqual(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dpRef,
                                                const DynamicsProcessing& dpTest) {
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigEqual(dpRef.get<DynamicsProcessing::engineArchitecture>(),
                                       dpTest.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::preEq>(),
                                            dpTest.get<DynamicsProcessing::preEq>(),
                                            mEngineConfig.preEqBand.inUse);
        }
        case DynamicsProcessing::postEq: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::postEq>(),
                                            dpTest.get<DynamicsProcessing::postEq>(),
                                            mEngineConfig.postEqBand.inUse);
        }
        case DynamicsProcessing::mbc: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::mbc>(),
                                            dpTest.get<DynamicsProcessing::mbc>(),
                                            mEngineConfig.mbcBand.inUse);
        }
        case DynamicsProcessing::preEqBand: {
            return isEqBandConfigEqual(dpRef.get<DynamicsProcessing::preEqBand>(),
                                       dpTest.get<DynamicsProcessing::preEqBand>(),
                                       mEngineConfig.preEqBand.inUse);
        }
        case DynamicsProcessing::postEqBand: {
            return isEqBandConfigEqual(dpRef.get<DynamicsProcessing::postEqBand>(),
                                       dpTest.get<DynamicsProcessing::postEqBand>(),
                                       mEngineConfig.postEqBand.inUse);
        }
        case DynamicsProcessing::mbcBand: {
            return isMbcBandConfigEqual(dpRef.get<DynamicsProcessing::mbcBand>(),
                                        dpTest.get<DynamicsProcessing::mbcBand>(),
                                        mEngineConfig.mbcBand.inUse);
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigEqual(dpRef.get<DynamicsProcessing::limiter>(),
                                        dpTest.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGainDb: {
            return dpRef.get<DynamicsProcessing::inputGainDb>() ==
                   dpTest.get<DynamicsProcessing::inputGainDb>();
        }
        case DynamicsProcessing::vendorExtension: {
            return false;
        }
    }
    return false;
}

bool DynamicsProcessingTestHelper::isEngineConfigValid(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    if (cfg.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION &&
        cfg.resolutionPreference !=
                DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION) {
        return false;
    }
    if (cfg.preferredFrameDurationMs < 0) return false;
    if ((cfg.preEqBand.inUse && cfg.preEqBand.bandCount <= 0) ||
        (cfg.postEqBand.inUse && cfg.postEqBand.bandCount <= 0) ||
        (cfg.mbcBand.inUse && cfg.mbcBand.bandCount <= 0)) {
        return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isPreEqBandChannelConfigValid(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (cfg.enablement.inUse != mEngineConfig.preEqBand.inUse) return false;
        if (cfg.enablement.inUse) {
            if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
            if (cfg.enablement.bandCount != mEngineConfig.preEqBand.bandCount) return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isPostEqBandChannelConfigValid(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (cfg.enablement.inUse != mEngineConfig.postEqBand.inUse) return false;
        if (cfg.enablement.inUse) {
            if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
            if (cfg.enablement.bandCount != mEngineConfig.postEqBand.bandCount) return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isMbcBandChannelConfigValid(
        const std::vector<DynamicsProcessing::BandChannelConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (cfg.enablement.inUse != mEngineConfig.mbcBand.inUse) return false;
        if (cfg.enablement.inUse) {
            if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
            if (cfg.enablement.bandCount != mEngineConfig.mbcBand.bandCount) return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isEqBandConfigValid(
        const DynamicsProcessing::Capability& cap,
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs, bool stageInUse, int bandCount) {
    if (!stageInUse) return true;
    std::vector<float> freqs(cfgs.size(), -1);
    for (auto cfg : cfgs) {
        if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;
        if (cfg.cutoffFrequency < cap.minCutOffFreq || cfg.cutoffFrequency > cap.maxCutOffFreq) {
            return false;
        }
        freqs[cfg.band] = cfg.cutoffFrequency;
    }
    if (std::count(freqs.begin(), freqs.end(), -1)) return false;
    return std::is_sorted(freqs.begin(), freqs.end());
}

bool DynamicsProcessingTestHelper::isMbcBandConfigValid(
        const DynamicsProcessing::Capability& cap,
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs, bool stageInUse,
        int bandCount) {
    if (!stageInUse) return true;
    std::vector<float> freqs(cfgs.size(), -1);
    for (auto cfg : cfgs) {
        if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;
        if (cfg.cutoffFrequencyHz < cap.minCutOffFreq ||
            cfg.cutoffFrequencyHz > cap.maxCutOffFreq) {
            return false;
        }
        if ((cfg.attackTimeMs < 0) || (cfg.releaseTimeMs < 0) || (cfg.ratio < 0) ||
            (cfg.thresholdDb > 0) || (cfg.kneeWidthDb < 0) || (cfg.noiseGateThresholdDb > 0) ||
            (cfg.expanderRatio < 0)) {
            return false;
        }
        freqs[cfg.band] = cfg.cutoffFrequencyHz;
    }
    if (std::count(freqs.begin(), freqs.end(), -1)) return false;
    return std::is_sorted(freqs.begin(), freqs.end());
}

bool DynamicsProcessingTestHelper::isLimiterConfigValid(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (cfg.inUse != mEngineConfig.limiterInUse) return false;
        if (cfg.inUse) {
            if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
            if (cfg.attackTimeMs < 0) return false;
            if (cfg.releaseTimeMs < 0) return false;
            if (cfg.ratio < 0) return false;
            if (cfg.thresholdDb > 0) return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isEngineConfigEqual(
        const DynamicsProcessing::EngineArchitecture& ref,
        const DynamicsProcessing::EngineArchitecture& test) {
    if (ref.resolutionPreference != test.resolutionPreference) return false;
    if (ref.preferredFrameDurationMs != test.preferredFrameDurationMs) return false;
    if (ref.preEqBand.inUse != test.preEqBand.inUse) return false;
    if (ref.preEqBand.inUse && (ref.preEqBand.bandCount != test.preEqBand.bandCount)) {
        return false;
    }
    if (ref.postEqBand.inUse != test.postEqBand.inUse) return false;
    if (ref.postEqBand.inUse && (ref.postEqBand.bandCount != test.postEqBand.bandCount)) {
        return false;
    }
    if (ref.mbcBand.inUse != test.mbcBand.inUse) return false;
    if (ref.mbcBand.inUse && (ref.mbcBand.bandCount != test.mbcBand.bandCount)) {
        return false;
    }
    if (ref.limiterInUse != test.limiterInUse) return false;
    return true;
}

bool DynamicsProcessingTestHelper::isBandChannelConfigEqual(
        const DynamicsProcessing::BandChannelConfig& refCfg,
        const DynamicsProcessing::BandChannelConfig& testCfg, bool stageInUse) {
    if (!stageInUse) return true;
    if (refCfg.enablement.inUse != testCfg.enablement.inUse) return false;
    if (refCfg.enablement.inUse) return refCfg == testCfg;
    return true;
}

bool DynamicsProcessingTestHelper::isBandChannelConfigEqual(
        const std::vector<DynamicsProcessing::BandChannelConfig>& refCfgs,
        const std::vector<DynamicsProcessing::BandChannelConfig>& testCfgs, bool stageInUse) {
    if (!stageInUse) return true;
    for (size_t i = 0; i < refCfgs.size(); i++) {
        size_t j;
        for (j = 0; j < testCfgs.size(); j++) {
            if (refCfgs[i].channel == testCfgs[j].channel) break;
        }
        if (j == testCfgs.size() ||
            !isBandChannelConfigEqual(refCfgs[i], testCfgs[j], stageInUse)) {
            return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isEqBandConfigEqual(
        const DynamicsProcessing::EqBandConfig& refCfg,
        const DynamicsProcessing::EqBandConfig& testCfg, bool stageInUse) {
    if (!stageInUse) return true;
    return refCfg == testCfg;
}

bool DynamicsProcessingTestHelper::isEqBandConfigEqual(
        const std::vector<DynamicsProcessing::EqBandConfig>& refCfgs,
        const std::vector<DynamicsProcessing::EqBandConfig>& testCfgs, bool stageInUse) {
    if (!stageInUse) return true;
    for (size_t i = 0; i < refCfgs.size(); i++) {
        size_t j;
        for (j = 0; j < testCfgs.size(); j++) {
            if (refCfgs[i].channel == testCfgs[j].channel && refCfgs[i].band == testCfgs[j].band) {
                break;
            }
        }
        if (j == testCfgs.size() || !isEqBandConfigEqual(refCfgs[i], testCfgs[j], stageInUse)) {
            return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isMbcBandConfigEqual(
        const DynamicsProcessing::MbcBandConfig& refCfg,
        const DynamicsProcessing::MbcBandConfig& testCfg, bool stageInUse) {
    if (!stageInUse) return true;
    return refCfg == testCfg;
}

bool DynamicsProcessingTestHelper::isMbcBandConfigEqual(
        const std::vector<DynamicsProcessing::MbcBandConfig>& refCfgs,
        const std::vector<DynamicsProcessing::MbcBandConfig>& testCfgs, bool stageInUse) {
    if (!stageInUse) return true;
    for (size_t i = 0; i < refCfgs.size(); i++) {
        size_t j;
        for (j = 0; j < testCfgs.size(); j++) {
            if (refCfgs[i].channel == testCfgs[j].channel && refCfgs[i].band == testCfgs[j].band) {
                break;
            }
        }
        if (j == testCfgs.size() || !isMbcBandConfigEqual(refCfgs[i], testCfgs[j], stageInUse)) {
            return false;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isLimiterConfigEqual(
        const DynamicsProcessing::LimiterConfig& refCfg,
        const DynamicsProcessing::LimiterConfig& testCfg) {
    if (refCfg.inUse != testCfg.inUse) return false;
    if (refCfg.inUse) return refCfg == testCfg;
    return true;
}

bool DynamicsProcessingTestHelper::isLimiterConfigEqual(
        const std::vector<DynamicsProcessing::LimiterConfig>& refCfgs,
        const std::vector<DynamicsProcessing::LimiterConfig>& testCfgs) {
    if (!mEngineConfig.limiterInUse) return true;
    for (size_t i = 0; i < refCfgs.size(); i++) {
        size_t j;
        for (j = 0; j < testCfgs.size(); j++) {
            if (refCfgs[i].channel == testCfgs[j].channel) break;
        }
        if (j == testCfgs.size() || !isLimiterConfigEqual(refCfgs[i], testCfgs[j])) {
            return false;
        }
    }
    return true;
}

void DynamicsProcessingTestHelper::SetAndGetDynamicsProcessingParameters() {
    for (auto& it : mTags) {
        auto& tag = it.first;
        auto& dp = it.second;

        // validate parameter
        Descriptor desc;
        ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
        const bool valid = isParamValid(tag, dp, desc);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::dynamicsProcessing>(dp);
        expectParam.set<Parameter::specific>(specific);
        EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

        // only get if parameter in range and set success
        if (expected == EX_NONE) {
            Parameter getParam;
            Parameter::Id id;
            DynamicsProcessing::Id dpId;
            dpId.set<DynamicsProcessing::Id::commonTag>(tag);
            id.set<Parameter::Id::dynamicsProcessingTag>(dpId);
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            Parameter::Specific specificTest = getParam.get<Parameter::specific>();
            EXPECT_TRUE(isParamEqual(tag, dp,
                                     specificTest.get<Parameter::Specific::dynamicsProcessing>()));
        }
    }
}

void DynamicsProcessingTestHelper::addEngineConfig(DynamicsProcessing::EngineArchitecture& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::engineArchitecture>(cfg);
    mTags.push_back({DynamicsProcessing::engineArchitecture, dp});
}

void DynamicsProcessingTestHelper::addPreEqBandChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEq>(std::vector<DynamicsProcessing::BandChannelConfig>{cfg});
    mTags.push_back({DynamicsProcessing::preEq, dp});
}

void DynamicsProcessingTestHelper::addPostEqBandChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEq>(std::vector<DynamicsProcessing::BandChannelConfig>{cfg});
    mTags.push_back({DynamicsProcessing::postEq, dp});
}

void DynamicsProcessingTestHelper::addMbcBandChannelConfig(
        DynamicsProcessing::BandChannelConfig& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbc>(std::vector<DynamicsProcessing::BandChannelConfig>{cfg});
    mTags.push_back({DynamicsProcessing::mbc, dp});
}

void DynamicsProcessingTestHelper::addPreEqBandConfigs(
        std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::preEqBand, dp});
}

void DynamicsProcessingTestHelper::addPostEqBandConfigs(
        std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::postEqBand, dp});
}

void DynamicsProcessingTestHelper::addMbcBandConfigs(
        std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbcBand>(cfgs);
    mTags.push_back({DynamicsProcessing::mbcBand, dp});
}

void DynamicsProcessingTestHelper::addLimiterConfig(DynamicsProcessing::LimiterConfig& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::limiter>(std::vector<DynamicsProcessing::LimiterConfig>{cfg});
    mTags.push_back({DynamicsProcessing::limiter, dp});
}

void DynamicsProcessingTestHelper::addInputGain(float inputGaindB) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::inputGainDb>(inputGaindB);
    mTags.push_back({DynamicsProcessing::inputGainDb, dp});
}

/**
 * Test DynamicsProcessing Engine Configuration
 */
class DynamicsProcessingTestEngineArchitecture
    : public ::testing::TestWithParam<std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                 DynamicsProcessing::ResolutionPreference, float,
                                                 bool, int, bool, int, bool, int, bool>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEngineArchitecture()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mVariant(std::get<1>(GetParam())),
          mFrameDurationMs(std::get<2>(GetParam())),
          mUsePreEq(std::get<3>(GetParam())),
          mPreEqBandCount(std::get<4>(GetParam())),
          mUsePostEq(std::get<5>(GetParam())),
          mPostEqBandCount(std::get<6>(GetParam())),
          mUseMbc(std::get<7>(GetParam())),
          mMbcBandCount(std::get<8>(GetParam())),
          mUseLimiter(std::get<9>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const DynamicsProcessing::ResolutionPreference mVariant;
    const float mFrameDurationMs;
    const bool mUsePreEq;
    const int mPreEqBandCount;
    const bool mUsePostEq;
    const int mPostEqBandCount;
    const bool mUseMbc;
    const int mMbcBandCount;
    const bool mUseLimiter;
};

TEST_P(DynamicsProcessingTestEngineArchitecture, SetAndGetEngineArch) {
    DynamicsProcessing::EngineArchitecture cfg{mVariant,
                                               mFrameDurationMs,
                                               {mUsePreEq, mPreEqBandCount},
                                               {mUsePostEq, mPostEqBandCount},
                                               {mUseMbc, mMbcBandCount},
                                               mUseLimiter};
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(cfg));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEngineArchitecture,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                testing::Values(static_cast<DynamicsProcessing::ResolutionPreference>(16),
                                DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION,
                                DynamicsProcessing::ResolutionPreference::
                                        FAVOR_FREQUENCY_RESOLUTION),  // variant
                testing::Values(-10.f, 0.f, 10.f),                    // frame duration
                testing::Bool(), testing::Values(-1, 0, 5),           // pre eq enablement
                testing::Bool(), testing::Values(-1, 0, 5),           // post eq enablement
                testing::Bool(), testing::Values(-1, 0, 5),           // mbc enablement
                testing::Bool()),                                     // limiter enable
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            DynamicsProcessing::EngineArchitecture cfg;
            cfg.resolutionPreference = std::get<1>(info.param);
            cfg.preferredFrameDurationMs = std::get<2>(info.param);
            cfg.preEqBand.inUse = std::get<3>(info.param);
            cfg.preEqBand.bandCount = std::get<4>(info.param);
            cfg.postEqBand.inUse = std::get<5>(info.param);
            cfg.postEqBand.bandCount = std::get<6>(info.param);
            cfg.mbcBand.inUse = std::get<7>(info.param);
            cfg.mbcBand.bandCount = std::get<8>(info.param);
            cfg.limiterInUse = std::get<9>(info.param);
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_Cfg_" + cfg.toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEngineArchitecture);

/**
 * Test DynamicsProcessing Input Gain
 */
class DynamicsProcessingTestInputGain
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, float>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestInputGain()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mInputGain(std::get<1>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const float mInputGain;
};

TEST_P(DynamicsProcessingTestInputGain, SetAndGetInputGain) {
    EXPECT_NO_FATAL_FAILURE(addInputGain(mInputGain));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestInputGain,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                           testing::Values(-6.f, 0.f, 6.f)),  // gains
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string gain = std::to_string(std::get<1>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_gain_" + gain;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestInputGain);

/**
 * Test DynamicsProcessing Limiter Config
 */
class DynamicsProcessingTestLimiterConfig
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t, bool, bool,
                         int32_t, float, float, float, float, float, bool>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestLimiterConfig()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mChannel(std::get<1>(GetParam())),
          mEnable(std::get<2>(GetParam())),
          mInUse(std::get<3>(GetParam())),
          mLinkGroup(std::get<4>(GetParam())),
          mAttackTimeMs(std::get<5>(GetParam())),
          mReleaseTimeMs(std::get<6>(GetParam())),
          mRatio(std::get<7>(GetParam())),
          mThresholdDb(std::get<8>(GetParam())),
          mPostGainDb(std::get<9>(GetParam())),
          mInUseEngine(std::get<10>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const int32_t mChannel;
    const bool mEnable;
    const bool mInUse;
    const int32_t mLinkGroup;
    const float mAttackTimeMs;
    const float mReleaseTimeMs;
    const float mRatio;
    const float mThresholdDb;
    const float mPostGainDb;
    const bool mInUseEngine;
};

TEST_P(DynamicsProcessingTestLimiterConfig, SetAndGetLimiterConfig) {
    DynamicsProcessing::LimiterConfig cfg{mChannel,   mEnable,       mInUse,
                                          mLinkGroup, mAttackTimeMs, mReleaseTimeMs,
                                          mRatio,     mThresholdDb,  mPostGainDb};
    mEngineConfig.limiterInUse = mInUseEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    EXPECT_NO_FATAL_FAILURE(addLimiterConfig(cfg));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestLimiterConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                           testing::Values(-1, 0, 1, 2),    // channel count
                           testing::Bool(),                 // enable
                           testing::Bool(),                 // inuse
                           testing::Values(3),              // link group
                           testing::Values(-1, 1),          // attack time ms
                           testing::Values(-60, 60),        // release time ms
                           testing::Values(-2.5, 2.5),      // ratio
                           testing::Values(-2, 2),          // threshold
                           testing::Values(-3.14f, 3.14f),  // post gain
                           testing::Bool()),                // engine limiter enable
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            DynamicsProcessing::LimiterConfig cfg;
            cfg.channel = std::get<1>(info.param);
            cfg.enable = std::get<2>(info.param);
            cfg.inUse = std::get<3>(info.param);
            cfg.linkGroup = std::get<4>(info.param);
            cfg.attackTimeMs = std::get<5>(info.param);
            cfg.releaseTimeMs = std::get<6>(info.param);
            cfg.ratio = std::get<7>(info.param);
            cfg.thresholdDb = std::get<8>(info.param);
            cfg.postGainDb = std::get<9>(info.param);
            std::string engineLimiterInUse = std::to_string(std::get<10>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_Cfg_" + cfg.toString() +
                               "_engineSetting_" + engineLimiterInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestLimiterConfig);

/**
 * Test DynamicsProcessing BandChannelConfig
 */
class DynamicsProcessingTestBandChannelConfig
    : public ::testing::TestWithParam<std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                 int32_t, bool, int32_t, bool, int32_t>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestBandChannelConfig()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mChannel(std::get<1>(GetParam())),
          mInUse(std::get<2>(GetParam())),
          mBandCount(std::get<3>(GetParam())),
          mInUseEngine(std::get<4>(GetParam())),
          mBandCountEngine(std::get<5>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const int32_t mChannel;
    const bool mInUse;
    const int32_t mBandCount;
    const bool mInUseEngine;
    const int32_t mBandCountEngine;
};

TEST_P(DynamicsProcessingTestBandChannelConfig, SetAndGetPreEqBandChannelConfig) {
    DynamicsProcessing::BandChannelConfig cfg{mChannel, {mInUse, mBandCount}};
    mEngineConfig.preEqBand.inUse = mInUseEngine;
    mEngineConfig.preEqBand.bandCount = mBandCountEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    EXPECT_NO_FATAL_FAILURE(addPreEqBandChannelConfig(cfg));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestBandChannelConfig, SetAndGetPostEqBandChannelConfig) {
    DynamicsProcessing::BandChannelConfig cfg{mChannel, {mInUse, mBandCount}};
    mEngineConfig.postEqBand.inUse = mInUseEngine;
    mEngineConfig.postEqBand.bandCount = mBandCountEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    EXPECT_NO_FATAL_FAILURE(addPostEqBandChannelConfig(cfg));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestBandChannelConfig, SetAndGetMbcBandChannelConfig) {
    DynamicsProcessing::BandChannelConfig cfg{mChannel, {mInUse, mBandCount}};
    mEngineConfig.mbcBand.inUse = mInUseEngine;
    mEngineConfig.mbcBand.bandCount = mBandCountEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    EXPECT_NO_FATAL_FAILURE(addMbcBandChannelConfig(cfg));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestBandChannelConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                           testing::Values(-1, 0, 1, 2),  // channel count
                           testing::Bool(),               // inuse
                           testing::Values(0, 5, 6),      // band count
                           testing::Bool(),               // engine in use
                           testing::Values(5, 6)),        // engine band count
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            DynamicsProcessing::BandChannelConfig cfg;
            cfg.channel = std::get<1>(info.param);
            cfg.enablement.inUse = std::get<2>(info.param);
            cfg.enablement.bandCount = std::get<3>(info.param);
            std::string engineInUse = std::to_string(std::get<4>(info.param));
            std::string engineBandCount = std::to_string(std::get<5>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_Cfg_" + cfg.toString() +
                               "_engineSetting_" + engineInUse + "_" + engineBandCount;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestBandChannelConfig);

/**
 * Test DynamicsProcessing EqBandConfig
 */
class DynamicsProcessingTestEqBandConfig
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t, bool,
                         std::vector<std::pair<int, float>>, float, bool>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEqBandConfig()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mChannel(std::get<1>(GetParam())),
          mEnable(std::get<2>(GetParam())),
          mCutOffFreqs(std::get<3>(GetParam())),
          mGain(std::get<4>(GetParam())),
          mStageInUse(std::get<5>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const int32_t mChannel;
    const bool mEnable;
    const std::vector<std::pair<int, float>> mCutOffFreqs;
    const float mGain;
    const bool mStageInUse;
};

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPreEqBandConfig) {
    int bandCount = mCutOffFreqs.size();
    mEngineConfig.preEqBand.inUse = mStageInUse;
    mEngineConfig.preEqBand.bandCount = bandCount;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    DynamicsProcessing::BandChannelConfig cfg{
            0, {mEngineConfig.preEqBand.inUse, mEngineConfig.preEqBand.bandCount}};
    EXPECT_NO_FATAL_FAILURE(addPreEqBandChannelConfig(cfg));
    std::vector<DynamicsProcessing::EqBandConfig> cfgs(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i].channel = mChannel;
        cfgs[i].band = mCutOffFreqs[i].first;
        cfgs[i].enable = mEnable;
        cfgs[i].cutoffFrequency = mCutOffFreqs[i].second;
        cfgs[i].gain = mGain;
    }
    EXPECT_NO_FATAL_FAILURE(addPreEqBandConfigs(cfgs));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPostEqBandConfig) {
    int bandCount = mCutOffFreqs.size();
    mEngineConfig.postEqBand.inUse = mStageInUse;
    mEngineConfig.postEqBand.bandCount = bandCount;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    DynamicsProcessing::BandChannelConfig cfg{
            0, {mEngineConfig.postEqBand.inUse, mEngineConfig.postEqBand.bandCount}};
    EXPECT_NO_FATAL_FAILURE(addPostEqBandChannelConfig(cfg));
    std::vector<DynamicsProcessing::EqBandConfig> cfgs(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i].channel = mChannel;
        cfgs[i].band = mCutOffFreqs[i].first;
        cfgs[i].enable = mEnable;
        cfgs[i].cutoffFrequency = mCutOffFreqs[i].second;
        cfgs[i].gain = mGain;
    }
    EXPECT_NO_FATAL_FAILURE(addPostEqBandConfigs(cfgs));
    SetAndGetDynamicsProcessingParameters();
}

std::vector<std::vector<std::pair<int, float>>> kBands{
        {
                {0, 600},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {4, 16000},
        },  // 5 bands
        {
                {0, 800},
                {3, 15000},
                {2, 6000},
                {1, 2000},
        },  // 4 bands, unsorted
        {
                {0, 650},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {3, 16000},
        },  // 5 bands, missing band
        {
                {0, 900},
                {1, 8000},
                {2, 4000},
                {3, 12000},
        },  // 4 bands, cutoff freq not increasing
        {
                {0, 450},
                {1, 2000},
                {7, 6000},
                {3, 10000},
                {4, 16000},
        },  // bad band index
        {
                {0, 1},
                {1, 8000},
        },  // too low cutoff freq
        {
                {0, 1200},
                {1, 80000},
        },  // too high cutoff freq
};

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEqBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                           testing::Values(-1, 0, 10),      // channel count
                           testing::Bool(),                 // enable
                           testing::ValuesIn(kBands),       // cut off frequencies
                           testing::Values(-3.14f, 3.14f),  // gain
                           testing::Bool()),                // stage in use
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            DynamicsProcessing::EqBandConfig cfg;
            cfg.channel = std::get<1>(info.param);
            cfg.enable = std::get<2>(info.param);
            cfg.band = std::get<3>(info.param)[0].first;
            cfg.cutoffFrequency = std::get<3>(info.param)[0].second;
            cfg.gain = std::get<4>(info.param);
            std::string stageInUse = std::to_string(std::get<5>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_Cfg_" + cfg.toString() +
                               "_stageInUse_" + stageInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEqBandConfig);

/**
 * Test DynamicsProcessing MbcBandConfig
 */
class DynamicsProcessingTestMbcBandConfig
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t, bool,
                         std::vector<std::pair<int, float>>, float, float, float, float, float,
                         float, float, float, float, float, bool>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestMbcBandConfig()
        : DynamicsProcessingTestHelper(std::get<0>(GetParam())),
          mChannel(std::get<1>(GetParam())),
          mEnable(std::get<2>(GetParam())),
          mCutOffFreqs(std::get<3>(GetParam())),
          mGainDb(std::get<4>(GetParam())),
          mAttackTimeMs(std::get<5>(GetParam())),
          mReleaseTimeMs(std::get<6>(GetParam())),
          mRatio(std::get<7>(GetParam())),
          mThresholdDb(std::get<8>(GetParam())),
          mKneeWidthDb(std::get<9>(GetParam())),
          mNoiseGateThresholdDb(std::get<10>(GetParam())),
          mExpanderRatio(std::get<11>(GetParam())),
          mPreGainDb(std::get<12>(GetParam())),
          mPostGainDb(std::get<13>(GetParam())),
          mStageInUse(std::get<14>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const int32_t mChannel;
    const bool mEnable;
    const std::vector<std::pair<int, float>> mCutOffFreqs;
    const float mGainDb;
    const float mAttackTimeMs;
    const float mReleaseTimeMs;
    const float mRatio;
    const float mThresholdDb;
    const float mKneeWidthDb;
    const float mNoiseGateThresholdDb;
    const float mExpanderRatio;
    const float mPreGainDb;
    const float mPostGainDb;
    const bool mStageInUse;
};

TEST_P(DynamicsProcessingTestMbcBandConfig, SetAndGetMbcBandConfig) {
    int bandCount = mCutOffFreqs.size();
    mEngineConfig.mbcBand.inUse = mStageInUse;
    mEngineConfig.mbcBand.bandCount = bandCount;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfig));
    DynamicsProcessing::BandChannelConfig cfg{
            0, {mEngineConfig.mbcBand.inUse, mEngineConfig.mbcBand.bandCount}};
    EXPECT_NO_FATAL_FAILURE(addPostEqBandChannelConfig(cfg));
    std::vector<DynamicsProcessing::MbcBandConfig> cfgs(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i].channel = mChannel;
        cfgs[i].band = mCutOffFreqs[i].first;
        cfgs[i].enable = mEnable;
        cfgs[i].cutoffFrequencyHz = mCutOffFreqs[i].second;
        cfgs[i].gainDb = mGainDb;
        cfgs[i].attackTimeMs = mAttackTimeMs;
        cfgs[i].releaseTimeMs = mReleaseTimeMs;
        cfgs[i].ratio = mRatio;
        cfgs[i].thresholdDb = mThresholdDb;
        cfgs[i].kneeWidthDb = mKneeWidthDb;
        cfgs[i].noiseGateThresholdDb = mNoiseGateThresholdDb;
        cfgs[i].expanderRatio = mExpanderRatio;
        cfgs[i].preGainDb = mPreGainDb;
        cfgs[i].postGainDb = mPostGainDb;
    }
    EXPECT_NO_FATAL_FAILURE(addMbcBandConfigs(cfgs));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestMbcBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kDynamicsProcessingTypeUUID)),
                           testing::Values(-1, 0, 10),      // channel count
                           testing::Bool(),                 // enable
                           testing::ValuesIn(kBands),       // cut off frequencies
                           testing::Values(-3.14f, 3.14f),  // gain
                           testing::Values(-3, 3),          // attack time ms
                           testing::Values(-80, 80),        // release time ms
                           testing::Values(-2.5, 2.5),      // ratio
                           testing::Values(-2, 2),          // threshold
                           testing::Values(-5, 5),          // knee width
                           testing::Values(-90, 90),        // noise gate threshold
                           testing::Values(-2.5, 2.5),      // expander ratio
                           testing::Values(-2, 2),          // pre gain
                           testing::Values(-2, 2),          // post gain
                           testing::Bool()),                // stage in use
        [](const auto& info) {
            auto descriptor = std::get<0>(info.param).second;
            DynamicsProcessing::MbcBandConfig cfg;
            cfg.channel = std::get<1>(info.param);
            cfg.band = std::get<3>(info.param)[0].first;
            cfg.enable = std::get<2>(info.param);
            cfg.cutoffFrequencyHz = std::get<3>(info.param)[0].second;
            cfg.gainDb = std::get<4>(info.param);
            cfg.attackTimeMs = std::get<5>(info.param);
            cfg.releaseTimeMs = std::get<6>(info.param);
            cfg.ratio = std::get<7>(info.param);
            cfg.thresholdDb = std::get<8>(info.param);
            cfg.kneeWidthDb = std::get<9>(info.param);
            cfg.noiseGateThresholdDb = std::get<10>(info.param);
            cfg.expanderRatio = std::get<11>(info.param);
            cfg.preGainDb = std::get<12>(info.param);
            cfg.postGainDb = std::get<13>(info.param);
            std::string stageInUse = std::to_string(std::get<14>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_Cfg_" + cfg.toString() +
                               "_stageInUse_" + stageInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestMbcBandConfig);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
