/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <set>
#include <string>
#include <unordered_set>

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalDynamicsProcessingTest"
#include <android-base/logging.h>

#include <Utils.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessing;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDynamicsProcessing;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
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
        mChannelCount = ::aidl::android::hardware::audio::common::getChannelCount(
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
        mEngineConfigApplied = mEngineConfigPreset;
    }

    Parameter::Specific getDefaultParamSpecific() {
        DynamicsProcessing dp = DynamicsProcessing::make<DynamicsProcessing::engineArchitecture>(
                mEngineConfigPreset);
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
    bool isParamEqual(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dpRef,
                      const DynamicsProcessing& dpTest);
    bool isEngineConfigEqual(const DynamicsProcessing::EngineArchitecture& refCfg,
                             const DynamicsProcessing::EngineArchitecture& testCfg);

    template <typename T>
    std::vector<T> filterEnabledVector(const std::vector<T>& vec);

    template <typename T>
    bool isAidlVectorEqualAfterFilter(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isAidlVectorEqual(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isInRange(const T& value, const T& low, const T& high) {
        return (value >= low) && (value <= high);
    }
    template <typename T, std::size_t... Is>
    bool isTupleInRange(const T& test, const T& min, const T& max, std::index_sequence<Is...>) {
        return (isInRange(std::get<Is>(test), std::get<Is>(min), std::get<Is>(max)) && ...);
    }
    template <typename T, std::size_t TupSize = std::tuple_size_v<T>>
    bool isTupleInRange(const T& test, const T& min, const T& max) {
        return isTupleInRange(test, min, max, std::make_index_sequence<TupSize>{});
    }
    template <typename T, typename F>
    bool isTupleInRange(const std::vector<T>& cfgs, const T& min, const T& max, F func) {
        auto minT = func(min), maxT = func(max);
        for (auto cfg : cfgs) {
            if (!isTupleInRange(func(cfg), minT, maxT)) return false;
        }
        return true;
    }
    int locateMinMaxForTag(DynamicsProcessing::Tag tag,
                           const std::vector<Range::DynamicsProcessingRange>& kRanges);
    bool isParamInRange(const DynamicsProcessing& dp,
                        const std::vector<Range::DynamicsProcessingRange>& kRanges);
    bool isEngineConfigInRange(const DynamicsProcessing::EngineArchitecture& cfg,
                               const DynamicsProcessing::EngineArchitecture& min,
                               const DynamicsProcessing::EngineArchitecture& max);
    bool isChannelConfigInRange(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs,
                                const DynamicsProcessing::ChannelConfig& min,
                                const DynamicsProcessing::ChannelConfig& max);
    bool isEqBandConfigInRange(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                               const DynamicsProcessing::EqBandConfig& min,
                               const DynamicsProcessing::EqBandConfig& max);
    bool isMbcBandConfigInRange(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                                const DynamicsProcessing::MbcBandConfig& min,
                                const DynamicsProcessing::MbcBandConfig& max);
    bool isLimiterConfigInRange(const std::vector<DynamicsProcessing::LimiterConfig>& cfgs,
                                const DynamicsProcessing::LimiterConfig& min,
                                const DynamicsProcessing::LimiterConfig& max);
    bool isInputGainConfigInRange(const std::vector<DynamicsProcessing::InputGain>& cfgs,
                                  const DynamicsProcessing::InputGain& min,
                                  const DynamicsProcessing::InputGain& max);
    bool isParamValid(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dp);
    bool isChannelIndexValid(int channelCount);
    bool isChannelConfigValid(const std::vector<DynamicsProcessing::ChannelConfig>& cfgs);
    bool isEqBandConfigValid(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                             int bandCount);
    bool isMbcBandConfigValid(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                              int bandCount);
    bool isLimiterConfigValid(const std::vector<DynamicsProcessing::LimiterConfig>& cfgs);
    bool isInputGainConfigValid(const std::vector<DynamicsProcessing::InputGain>& cfgs);

    // get set params and validate
    void SetAndGetDynamicsProcessingParameters();

    // enqueue test parameters
    void addEngineConfig(const DynamicsProcessing::EngineArchitecture& cfg);
    void addPreEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPostEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addMbcChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPreEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addPostEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addMbcBandConfigs(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs);
    void addLimiterConfig(const std::vector<DynamicsProcessing::LimiterConfig>& cfg);
    void addInputGain(const std::vector<DynamicsProcessing::InputGain>& inputGain);

    static constexpr float kPreferredProcessingDurationMs = 10.0f;
    static constexpr int kBandCount = 5;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    DynamicsProcessing::EngineArchitecture mEngineConfigApplied;
    DynamicsProcessing::EngineArchitecture mEngineConfigPreset{
            .resolutionPreference =
                    DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
            .preferredProcessingDurationMs = kPreferredProcessingDurationMs,
            .preEqStage = {.inUse = true, .bandCount = kBandCount},
            .postEqStage = {.inUse = true, .bandCount = kBandCount},
            .mbcStage = {.inUse = true, .bandCount = kBandCount},
            .limiterInUse = true,
    };

    std::unordered_set<int /* channelId */> mPreEqChannelEnable;
    std::unordered_set<int /* channelId */> mPostEqChannelEnable;
    std::unordered_set<int /* channelId */> mMbcChannelEnable;
    std::unordered_set<int /* channelId */> mLimiterChannelEnable;
    static const std::set<std::vector<DynamicsProcessing::ChannelConfig>> kChannelConfigTestSet;
    static const std::set<DynamicsProcessing::StageEnablement> kStageEnablementTestSet;
    static const std::set<std::vector<DynamicsProcessing::InputGain>> kInputGainTestSet;

  protected:
    int mChannelCount;

  private:
    int32_t mChannelLayout;
    std::vector<std::pair<DynamicsProcessing::Tag, DynamicsProcessing>> mTags;
    void CleanUp() {
        mTags.clear();
        mPreEqChannelEnable.clear();
        mPostEqChannelEnable.clear();
        mMbcChannelEnable.clear();
        mLimiterChannelEnable.clear();
    }
};

// test value set for DynamicsProcessing::StageEnablement
const std::set<DynamicsProcessing::StageEnablement>
        DynamicsProcessingTestHelper::kStageEnablementTestSet = {
                {.inUse = true, .bandCount = DynamicsProcessingTestHelper::kBandCount},
                {.inUse = true, .bandCount = 0},
                {.inUse = true, .bandCount = -1},
                {.inUse = false, .bandCount = 0},
                {.inUse = false, .bandCount = -1},
                {.inUse = false, .bandCount = DynamicsProcessingTestHelper::kBandCount}};

// test value set for DynamicsProcessing::ChannelConfig
const std::set<std::vector<DynamicsProcessing::ChannelConfig>>
        DynamicsProcessingTestHelper::kChannelConfigTestSet = {
                {{.channel = -1, .enable = false},
                 {.channel = 0, .enable = true},
                 {.channel = 1, .enable = false},
                 {.channel = 2, .enable = true}},
                {{.channel = -1, .enable = false}, {.channel = 2, .enable = true}},
                {{.channel = 0, .enable = true}, {.channel = 1, .enable = true}}};

// test value set for DynamicsProcessing::InputGain
const std::set<std::vector<DynamicsProcessing::InputGain>>
        DynamicsProcessingTestHelper::kInputGainTestSet = {
                {{.channel = 0, .gainDb = 10.f},
                 {.channel = 1, .gainDb = 0.f},
                 {.channel = 2, .gainDb = -10.f}},
                {{.channel = -1, .gainDb = -10.f}, {.channel = -2, .gainDb = 10.f}},
                {{.channel = -1, .gainDb = 10.f}, {.channel = 0, .gainDb = -10.f}},
                {{.channel = 0, .gainDb = 10.f}, {.channel = 1, .gainDb = -10.f}}};

bool DynamicsProcessingTestHelper::isInputGainConfigInRange(
        const std::vector<DynamicsProcessing::InputGain>& cfgs,
        const DynamicsProcessing::InputGain& min, const DynamicsProcessing::InputGain& max) {
    auto func = [](const DynamicsProcessing::InputGain& arg) -> std::tuple<int32_t, float> {
        return {arg.channel, arg.gainDb};
    };
    return isTupleInRange(cfgs, min, max, func);
}

bool DynamicsProcessingTestHelper::isLimiterConfigInRange(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs,
        const DynamicsProcessing::LimiterConfig& min,
        const DynamicsProcessing::LimiterConfig& max) {
    auto func = [](const DynamicsProcessing::LimiterConfig& arg) {
        return std::make_tuple(arg.channel, arg.enable, arg.linkGroup, arg.attackTimeMs,
                               arg.releaseTimeMs, arg.ratio, arg.thresholdDb, arg.postGainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

bool DynamicsProcessingTestHelper::isMbcBandConfigInRange(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
        const DynamicsProcessing::MbcBandConfig& min,
        const DynamicsProcessing::MbcBandConfig& max) {
    auto func = [](const DynamicsProcessing::MbcBandConfig& arg) {
        return std::make_tuple(arg.channel, arg.band, arg.enable, arg.cutoffFrequencyHz,
                               arg.attackTimeMs, arg.releaseTimeMs, arg.ratio, arg.thresholdDb,
                               arg.kneeWidthDb, arg.noiseGateThresholdDb, arg.expanderRatio,
                               arg.preGainDb, arg.postGainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

bool DynamicsProcessingTestHelper::isEqBandConfigInRange(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
        const DynamicsProcessing::EqBandConfig& min, const DynamicsProcessing::EqBandConfig& max) {
    auto func = [](const DynamicsProcessing::EqBandConfig& arg) {
        return std::make_tuple(arg.channel, arg.band, arg.enable, arg.cutoffFrequencyHz,
                               arg.gainDb);
    };
    return isTupleInRange(cfgs, min, max, func);
}

bool DynamicsProcessingTestHelper::isChannelConfigInRange(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs,
        const DynamicsProcessing::ChannelConfig& min,
        const DynamicsProcessing::ChannelConfig& max) {
    auto func = [](const DynamicsProcessing::ChannelConfig& arg) {
        return std::make_tuple(arg.channel, arg.enable);
    };
    return isTupleInRange(cfgs, min, max, func);
}

bool DynamicsProcessingTestHelper::isEngineConfigInRange(
        const DynamicsProcessing::EngineArchitecture& cfg,
        const DynamicsProcessing::EngineArchitecture& min,
        const DynamicsProcessing::EngineArchitecture& max) {
    auto func = [](const DynamicsProcessing::EngineArchitecture& arg) {
        return std::make_tuple(arg.resolutionPreference, arg.preferredProcessingDurationMs,
                               arg.preEqStage.inUse, arg.preEqStage.bandCount,
                               arg.postEqStage.inUse, arg.postEqStage.bandCount, arg.mbcStage.inUse,
                               arg.mbcStage.bandCount, arg.limiterInUse);
    };
    return isTupleInRange(func(cfg), func(min), func(max));
}

int DynamicsProcessingTestHelper::locateMinMaxForTag(
        DynamicsProcessing::Tag tag, const std::vector<Range::DynamicsProcessingRange>& kRanges) {
    for (int i = 0; i < (int)kRanges.size(); i++) {
        if (tag == kRanges[i].min.getTag() && tag == kRanges[i].max.getTag()) {
            return i;
        }
    }
    return -1;
}

bool DynamicsProcessingTestHelper::isParamInRange(
        const DynamicsProcessing& dp, const std::vector<Range::DynamicsProcessingRange>& kRanges) {
    auto tag = dp.getTag();
    int i = locateMinMaxForTag(tag, kRanges);
    if (i == -1) return true;

    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigInRange(
                    dp.get<DynamicsProcessing::engineArchitecture>(),
                    kRanges[i].min.get<DynamicsProcessing::engineArchitecture>(),
                    kRanges[i].max.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::preEq>(),
                                          kRanges[i].min.get<DynamicsProcessing::preEq>()[0],
                                          kRanges[i].max.get<DynamicsProcessing::preEq>()[0]);
        }
        case DynamicsProcessing::postEq: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::postEq>(),
                                          kRanges[i].min.get<DynamicsProcessing::postEq>()[0],
                                          kRanges[i].max.get<DynamicsProcessing::postEq>()[0]);
        }
        case DynamicsProcessing::mbc: {
            return isChannelConfigInRange(dp.get<DynamicsProcessing::mbc>(),
                                          kRanges[i].min.get<DynamicsProcessing::mbc>()[0],
                                          kRanges[i].max.get<DynamicsProcessing::mbc>()[0]);
        }
        case DynamicsProcessing::preEqBand: {
            return isEqBandConfigInRange(dp.get<DynamicsProcessing::preEqBand>(),
                                         kRanges[i].min.get<DynamicsProcessing::preEqBand>()[0],
                                         kRanges[i].max.get<DynamicsProcessing::preEqBand>()[0]);
        }
        case DynamicsProcessing::postEqBand: {
            return isEqBandConfigInRange(dp.get<DynamicsProcessing::postEqBand>(),
                                         kRanges[i].min.get<DynamicsProcessing::postEqBand>()[0],
                                         kRanges[i].max.get<DynamicsProcessing::postEqBand>()[0]);
        }
        case DynamicsProcessing::mbcBand: {
            return isMbcBandConfigInRange(dp.get<DynamicsProcessing::mbcBand>(),
                                          kRanges[i].min.get<DynamicsProcessing::mbcBand>()[0],
                                          kRanges[i].max.get<DynamicsProcessing::mbcBand>()[0]);
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigInRange(dp.get<DynamicsProcessing::limiter>(),
                                          kRanges[i].min.get<DynamicsProcessing::limiter>()[0],
                                          kRanges[i].max.get<DynamicsProcessing::limiter>()[0]);
        }
        case DynamicsProcessing::inputGain: {
            return isInputGainConfigInRange(dp.get<DynamicsProcessing::inputGain>(),
                                            kRanges[i].min.get<DynamicsProcessing::inputGain>()[0],
                                            kRanges[i].max.get<DynamicsProcessing::inputGain>()[0]);
        }
        default: {
            return true;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isChannelIndexValid(int channelCount) {
    if (channelCount < 0 || channelCount >= mChannelCount) return false;
    return true;
}

bool DynamicsProcessingTestHelper::isChannelConfigValid(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (!isChannelIndexValid(cfg.channel)) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isLimiterConfigValid(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    for (auto cfg : cfgs) {
        if (!isChannelIndexValid(cfg.channel)) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isInputGainConfigValid(
        const std::vector<DynamicsProcessing::InputGain>& cfgs) {
    for (auto cfg : cfgs) {
        if (!isChannelIndexValid(cfg.channel)) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isEqBandConfigValid(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs, int bandCount) {
    std::vector<float> freqs(cfgs.size(), -1);
    for (auto cfg : cfgs) {
        if (!isChannelIndexValid(cfg.channel)) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;
        freqs[cfg.band] = cfg.cutoffFrequencyHz;
    }
    if (std::count(freqs.begin(), freqs.end(), -1)) return false;
    return std::is_sorted(freqs.begin(), freqs.end());
}

bool DynamicsProcessingTestHelper::isMbcBandConfigValid(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs, int bandCount) {
    std::vector<float> freqs(cfgs.size(), -1);
    for (auto cfg : cfgs) {
        if (!isChannelIndexValid(cfg.channel)) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;

        freqs[cfg.band] = cfg.cutoffFrequencyHz;
    }
    if (std::count(freqs.begin(), freqs.end(), -1)) return false;
    return std::is_sorted(freqs.begin(), freqs.end());
}

bool DynamicsProcessingTestHelper::isParamValid(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dp) {
    switch (tag) {
        case DynamicsProcessing::preEq: {
            if (!mEngineConfigApplied.preEqStage.inUse) return false;
            return isChannelConfigValid(dp.get<DynamicsProcessing::preEq>());
        }
        case DynamicsProcessing::postEq: {
            if (!mEngineConfigApplied.postEqStage.inUse) return false;
            return isChannelConfigValid(dp.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            if (!mEngineConfigApplied.mbcStage.inUse) return false;
            return isChannelConfigValid(dp.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            if (!mEngineConfigApplied.preEqStage.inUse) return false;
            return isEqBandConfigValid(dp.get<DynamicsProcessing::preEqBand>(),
                                       mEngineConfigApplied.preEqStage.bandCount);
        }
        case DynamicsProcessing::postEqBand: {
            if (!mEngineConfigApplied.postEqStage.inUse) return false;
            return isEqBandConfigValid(dp.get<DynamicsProcessing::postEqBand>(),
                                       mEngineConfigApplied.postEqStage.bandCount);
        }
        case DynamicsProcessing::mbcBand: {
            if (!mEngineConfigApplied.mbcStage.inUse) return false;
            return isMbcBandConfigValid(dp.get<DynamicsProcessing::mbcBand>(),
                                        mEngineConfigApplied.mbcStage.bandCount);
        }
        case DynamicsProcessing::limiter: {
            if (!mEngineConfigApplied.limiterInUse) return false;
            return isLimiterConfigValid(dp.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isInputGainConfigValid(dp.get<DynamicsProcessing::inputGain>());
        }
        default: {
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
            const auto& source = dpRef.get<DynamicsProcessing::preEq>();
            const auto& target = dpTest.get<DynamicsProcessing::preEq>();
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(source, target);
        }
        case DynamicsProcessing::postEq: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::postEq>(),
                    dpTest.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::mbc>(), dpTest.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::preEqBand>(),
                    dpTest.get<DynamicsProcessing::preEqBand>());
        }
        case DynamicsProcessing::postEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::postEqBand>(),
                    dpTest.get<DynamicsProcessing::postEqBand>());
        }
        case DynamicsProcessing::mbcBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::MbcBandConfig>(
                    dpRef.get<DynamicsProcessing::mbcBand>(),
                    dpTest.get<DynamicsProcessing::mbcBand>());
        }
        case DynamicsProcessing::limiter: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::LimiterConfig>(
                    dpRef.get<DynamicsProcessing::limiter>(),
                    dpTest.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isAidlVectorEqual<DynamicsProcessing::InputGain>(
                    dpRef.get<DynamicsProcessing::inputGain>(),
                    dpTest.get<DynamicsProcessing::inputGain>());
        }
        case DynamicsProcessing::vendor: {
            return false;
        }
    }
}

bool DynamicsProcessingTestHelper::isEngineConfigEqual(
        const DynamicsProcessing::EngineArchitecture& ref,
        const DynamicsProcessing::EngineArchitecture& test) {
    return ref == test;
}

template <typename T>
std::vector<T> DynamicsProcessingTestHelper::filterEnabledVector(const std::vector<T>& vec) {
    std::vector<T> ret;
    std::copy_if(vec.begin(), vec.end(), std::back_inserter(ret),
                 [](const auto& v) { return v.enable; });
    return ret;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqual(const std::vector<T>& source,
                                                     const std::vector<T>& target) {
    if (source.size() != target.size()) return false;

    auto tempS = source;
    auto tempT = target;
    std::sort(tempS.begin(), tempS.end());
    std::sort(tempT.begin(), tempT.end());
    return tempS == tempT;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqualAfterFilter(const std::vector<T>& source,
                                                                const std::vector<T>& target) {
    return isAidlVectorEqual<T>(filterEnabledVector<T>(source), filterEnabledVector<T>(target));
}

void DynamicsProcessingTestHelper::SetAndGetDynamicsProcessingParameters() {
    for (auto& it : mTags) {
        auto& tag = it.first;
        auto& dp = it.second;

        // validate parameter
        Descriptor desc;
        ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
        bool valid = isParamInRange(dp, desc.capability.range.get<Range::dynamicsProcessing>());
        if (valid) valid = isParamValid(tag, dp);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::dynamicsProcessing>(dp);
        expectParam.set<Parameter::specific>(specific);
        ASSERT_STATUS(expected, mEffect->setParameter(expectParam))
                << "\n"
                << expectParam.toString() << "\n"
                << desc.toString();

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
            const auto& target = specificTest.get<Parameter::Specific::dynamicsProcessing>();
            EXPECT_TRUE(isParamEqual(tag, dp, target)) << dp.toString() << "\n"
                                                       << target.toString();
            // update mEngineConfigApplied after setting successfully
            if (tag == DynamicsProcessing::engineArchitecture) {
                mEngineConfigApplied = target.get<DynamicsProcessing::engineArchitecture>();
            }
        }
    }
}

void DynamicsProcessingTestHelper::addEngineConfig(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::engineArchitecture>(cfg);
    mTags.push_back({DynamicsProcessing::engineArchitecture, dp});
}

void DynamicsProcessingTestHelper::addPreEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEq>(cfgs);
    mTags.push_back({DynamicsProcessing::preEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPreEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPostEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEq>(cfgs);
    mTags.push_back({DynamicsProcessing::postEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPostEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addMbcChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbc>(cfgs);
    mTags.push_back({DynamicsProcessing::mbc, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mMbcChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPreEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::preEqBand, dp});
}

void DynamicsProcessingTestHelper::addPostEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::postEqBand, dp});
}

void DynamicsProcessingTestHelper::addMbcBandConfigs(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbcBand>(cfgs);
    mTags.push_back({DynamicsProcessing::mbcBand, dp});
}

void DynamicsProcessingTestHelper::addLimiterConfig(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::limiter>(cfgs);
    mTags.push_back({DynamicsProcessing::limiter, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mLimiterChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addInputGain(
        const std::vector<DynamicsProcessing::InputGain>& inputGains) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::inputGain>(inputGains);
    mTags.push_back({DynamicsProcessing::inputGain, dp});
}

/**
 * Test DynamicsProcessing Engine Configuration
 */
enum EngineArchitectureTestParamName {
    ENGINE_TEST_INSTANCE_NAME,
    ENGINE_TEST_RESOLUTION_PREFERENCE,
    ENGINE_TEST_PREFERRED_DURATION,
    ENGINE_TEST_STAGE_ENABLEMENT,
    ENGINE_TEST_LIMITER_IN_USE
};
using EngineArchitectureTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                DynamicsProcessing::ResolutionPreference, float,
                                                DynamicsProcessing::StageEnablement, bool>;

void fillEngineArchConfig(DynamicsProcessing::EngineArchitecture& cfg,
                          const EngineArchitectureTestParams& params) {
    cfg.resolutionPreference = std::get<ENGINE_TEST_RESOLUTION_PREFERENCE>(params);
    cfg.preferredProcessingDurationMs = std::get<ENGINE_TEST_PREFERRED_DURATION>(params);
    cfg.preEqStage = cfg.postEqStage = cfg.mbcStage =
            std::get<ENGINE_TEST_STAGE_ENABLEMENT>(params);
    cfg.limiterInUse = std::get<ENGINE_TEST_LIMITER_IN_USE>(params);
}

class DynamicsProcessingTestEngineArchitecture
    : public ::testing::TestWithParam<EngineArchitectureTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEngineArchitecture()
        : DynamicsProcessingTestHelper(std::get<ENGINE_TEST_INSTANCE_NAME>(GetParam())) {
        fillEngineArchConfig(mCfg, GetParam());
    };

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::EngineArchitecture mCfg;
};

TEST_P(DynamicsProcessingTestEngineArchitecture, SetAndGetEngineArch) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mCfg));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEngineArchitecture,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::Values(
                        DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION,
                        DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
                        static_cast<DynamicsProcessing::ResolutionPreference>(-1)),  // variant
                testing::Values(-10.f, 0.f, 10.f),  // processing duration
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kStageEnablementTestSet),  // preEQ/postEQ/mbc
                testing::Bool()),                                                // limiter enable
        [](const auto& info) {
            auto descriptor = std::get<ENGINE_TEST_INSTANCE_NAME>(info.param).second;
            DynamicsProcessing::EngineArchitecture cfg;
            fillEngineArchConfig(cfg, info.param);
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
enum InputGainTestParamName {
    INPUT_GAIN_INSTANCE_NAME,
    INPUT_GAIN_PARAM,
};
class DynamicsProcessingTestInputGain
    : public ::testing::TestWithParam<std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                 std::vector<DynamicsProcessing::InputGain>>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestInputGain()
        : DynamicsProcessingTestHelper(std::get<INPUT_GAIN_INSTANCE_NAME>(GetParam())),
          mInputGain(std::get<INPUT_GAIN_PARAM>(GetParam())){};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const std::vector<DynamicsProcessing::InputGain> mInputGain;
};

TEST_P(DynamicsProcessingTestInputGain, SetAndGetInputGain) {
    EXPECT_NO_FATAL_FAILURE(addInputGain(mInputGain));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestInputGain,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::ValuesIn(DynamicsProcessingTestInputGain::kInputGainTestSet)),
        [](const auto& info) {
            auto descriptor = std::get<INPUT_GAIN_INSTANCE_NAME>(info.param).second;
            std::string gains =
                    ::android::internal::ToString(std::get<INPUT_GAIN_PARAM>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_inputGains_" + gains;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestInputGain);

/**
 * Test DynamicsProcessing Limiter Config
 */
enum LimiterConfigTestParamName {
    LIMITER_INSTANCE_NAME,
    LIMITER_CHANNEL,
    LIMITER_ENABLE,
    LIMITER_LINK_GROUP,
    LIMITER_ENGINE_IN_USE,
    LIMITER_ADDITIONAL,
};
enum LimiterConfigTestAdditionalParam {
    LIMITER_ATTACK_TIME,
    LIMITER_RELEASE_TIME,
    LIMITER_RATIO,
    LIMITER_THRESHOLD,
    LIMITER_POST_GAIN,
    LIMITER_MAX_NUM,
};
using LimiterConfigTestAdditional = std::array<float, LIMITER_MAX_NUM>;
// attackTime, releaseTime, ratio, thresh, postGain
static constexpr std::array<LimiterConfigTestAdditional, 4> kLimiterConfigTestAdditionalParam = {
        {{-1, -60, -2.5, -2, -3.14},
         {-1, 60, -2.5, 2, -3.14},
         {1, -60, 2.5, -2, 3.14},
         {1, 60, 2.5, -2, 3.14}}};

using LimiterConfigTestParams =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t, bool, int32_t, bool,
                   LimiterConfigTestAdditional>;

void fillLimiterConfig(DynamicsProcessing::LimiterConfig& cfg,
                       const LimiterConfigTestParams& params) {
    const std::array<float, LIMITER_MAX_NUM> additional = std::get<LIMITER_ADDITIONAL>(params);
    cfg.channel = std::get<LIMITER_CHANNEL>(params);
    cfg.enable = std::get<LIMITER_ENABLE>(params);
    cfg.linkGroup = std::get<LIMITER_LINK_GROUP>(params);
    cfg.attackTimeMs = additional[LIMITER_ATTACK_TIME];
    cfg.releaseTimeMs = additional[LIMITER_RELEASE_TIME];
    cfg.ratio = additional[LIMITER_RATIO];
    cfg.thresholdDb = additional[LIMITER_THRESHOLD];
    cfg.postGainDb = additional[LIMITER_POST_GAIN];
}

class DynamicsProcessingTestLimiterConfig
    : public ::testing::TestWithParam<LimiterConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestLimiterConfig()
        : DynamicsProcessingTestHelper(std::get<LIMITER_INSTANCE_NAME>(GetParam())),
          mLimiterInUseEngine(std::get<LIMITER_ENGINE_IN_USE>(GetParam())) {
        fillLimiterConfig(mCfg, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::LimiterConfig mCfg;
    bool mLimiterInUseEngine;
};

TEST_P(DynamicsProcessingTestLimiterConfig, SetAndGetLimiterConfig) {
    mEngineConfigPreset.limiterInUse = mLimiterInUseEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addLimiterConfig({mCfg}));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestLimiterConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 1, 2),  // channel count
                           testing::Bool(),               // enable
                           testing::Values(3),            // link group
                           testing::Bool(),               // engine limiter enable
                           testing::ValuesIn(kLimiterConfigTestAdditionalParam)),  // Additional
        [](const auto& info) {
            auto descriptor = std::get<LIMITER_INSTANCE_NAME>(info.param).second;
            DynamicsProcessing::LimiterConfig cfg;
            fillLimiterConfig(cfg, info.param);
            std::string engineLimiterInUse =
                    std::to_string(std::get<LIMITER_ENGINE_IN_USE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_limiterConfig_" +
                               cfg.toString() + "_engineSetting_" + engineLimiterInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestLimiterConfig);

/**
 * Test DynamicsProcessing ChannelConfig
 */
enum ChannelConfigTestParamName {
    BAND_CHANNEL_TEST_INSTANCE_NAME,
    BAND_CHANNEL_TEST_CHANNEL_CONFIG,
    BAND_CHANNEL_TEST_ENGINE_IN_USE
};
using ChannelConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                           std::vector<DynamicsProcessing::ChannelConfig>, bool>;

class DynamicsProcessingTestChannelConfig
    : public ::testing::TestWithParam<ChannelConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestChannelConfig()
        : DynamicsProcessingTestHelper(std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(GetParam())),
          mCfg(std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(GetParam())),
          mInUseEngine(std::get<BAND_CHANNEL_TEST_ENGINE_IN_USE>(GetParam())) {}

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::ChannelConfig> mCfg;
    const bool mInUseEngine;
};

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPreEqChannelConfig) {
    mEngineConfigPreset.preEqStage.inUse = mInUseEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addPreEqChannelConfig(mCfg));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPostEqChannelConfig) {
    mEngineConfigPreset.postEqStage.inUse = mInUseEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addPostEqChannelConfig(mCfg));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetMbcChannelConfig) {
    mEngineConfigPreset.mbcStage.inUse = mInUseEngine;
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addMbcChannelConfig(mCfg));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestChannelConfig,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kChannelConfigTestSet),  // channel config
                testing::Bool()),                                              // Engine inUse
        [](const auto& info) {
            auto descriptor = std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(info.param).second;
            std::string engineInUse =
                    std::to_string(std::get<BAND_CHANNEL_TEST_ENGINE_IN_USE>(info.param));
            std::string channelConfig = ::android::internal::ToString(
                    std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_" + channelConfig +
                               "_engineInUse_" + engineInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestChannelConfig);

/**
 * Test DynamicsProcessing EqBandConfig
 */
enum EqBandConfigTestParamName {
    EQ_BAND_INSTANCE_NAME,
    EQ_BAND_CHANNEL,
    EQ_BAND_ENABLE,
    EQ_BAND_CUT_OFF_FREQ,
    EQ_BAND_GAIN,
    EQ_BAND_STAGE_IN_USE
};
using EqBandConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t,
                                          bool, std::vector<std::pair<int, float>>, float, bool>;

void fillEqBandConfig(std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                      const EqBandConfigTestParams& params) {
    const std::vector<std::pair<int, float>> cutOffFreqs = std::get<EQ_BAND_CUT_OFF_FREQ>(params);
    int bandCount = cutOffFreqs.size();
    cfgs.resize(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i].channel = std::get<EQ_BAND_CHANNEL>(params);
        cfgs[i].band = cutOffFreqs[i].first;
        cfgs[i].enable = std::get<EQ_BAND_ENABLE>(params);
        cfgs[i].cutoffFrequencyHz = cutOffFreqs[i].second;
        cfgs[i].gainDb = std::get<EQ_BAND_GAIN>(params);
    }
}

class DynamicsProcessingTestEqBandConfig : public ::testing::TestWithParam<EqBandConfigTestParams>,
                                           public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEqBandConfig()
        : DynamicsProcessingTestHelper(std::get<EQ_BAND_INSTANCE_NAME>(GetParam())),
          mStageInUse(std::get<EQ_BAND_STAGE_IN_USE>(GetParam())) {
        fillEqBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::EqBandConfig> mCfgs;
    const bool mStageInUse;
};

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPreEqBandConfig) {
    mEngineConfigPreset.preEqStage.inUse = mStageInUse;
    mEngineConfigPreset.preEqStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addPreEqChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addPreEqBandConfigs(mCfgs));
    SetAndGetDynamicsProcessingParameters();
}

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPostEqBandConfig) {
    mEngineConfigPreset.postEqStage.inUse = mStageInUse;
    mEngineConfigPreset.postEqStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addPostEqChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addPostEqBandConfigs(mCfgs));
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
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),      // channel ID
                           testing::Bool(),                 // band enable
                           testing::ValuesIn(kBands),       // cut off frequencies
                           testing::Values(-3.14f, 3.14f),  // gain
                           testing::Values(true)),          // stage in use
        [](const auto& info) {
            auto descriptor = std::get<EQ_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::EqBandConfig> cfgs;
            fillEqBandConfig(cfgs, info.param);
            std::string bands = ::android::internal::ToString(cfgs);
            std::string stageInUse = std::to_string(std::get<EQ_BAND_STAGE_IN_USE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_bands_" + bands +
                               "_stageInUse_" + stageInUse;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEqBandConfig);

/**
 * Test DynamicsProcessing MbcBandConfig
 */

enum MbcBandConfigParamName {
    MBC_BAND_INSTANCE_NAME,
    MBC_BAND_CHANNEL,
    MBC_BAND_ENABLE,
    MBC_BAND_CUTOFF_FREQ,
    MBC_BAND_STAGE_IN_USE,
    MBC_BAND_ADDITIONAL
};
enum MbcBandConfigAdditional {
    MBC_ADD_ATTACK_TIME,
    MBC_ADD_RELEASE_TIME,
    MBC_ADD_RATIO,
    MBC_ADD_THRESHOLD,
    MBC_ADD_KNEE_WIDTH,
    MBC_ADD_NOISE_GATE_THRESHOLD,
    MBC_ADD_EXPENDER_RATIO,
    MBC_ADD_PRE_GAIN,
    MBC_ADD_POST_GAIN,
    MBC_ADD_MAX_NUM
};
using TestParamsMbcBandConfigAdditional = std::array<float, MBC_ADD_MAX_NUM>;

// attackTime, releaseTime, ratio, thresh, kneeWidth, noise, expander, preGain, postGain
static constexpr std::array<TestParamsMbcBandConfigAdditional, 4> kMbcBandConfigAdditionalParam = {
        {{-3, -10, -2, -2, -5, -90, -2.5, -2, -2},
         {0, 0, 0, 0, 0, 0, 0, 0, 0},
         {-3, 10, -2, 2, -5, 90, -2.5, 2, -2},
         {3, 10, 2, -2, -5, 90, 2.5, 2, 2}}};

using TestParamsMbcBandConfig =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t, bool,
                   std::vector<std::pair<int, float>>, bool, TestParamsMbcBandConfigAdditional>;

void fillMbcBandConfig(std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                       const TestParamsMbcBandConfig& params) {
    const std::vector<std::pair<int, float>> cutOffFreqs = std::get<MBC_BAND_CUTOFF_FREQ>(params);
    const std::array<float, MBC_ADD_MAX_NUM> additional = std::get<MBC_BAND_ADDITIONAL>(params);
    int bandCount = cutOffFreqs.size();
    cfgs.resize(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i] = DynamicsProcessing::MbcBandConfig{
                .channel = std::get<MBC_BAND_CHANNEL>(params),
                .band = cutOffFreqs[i].first,
                .enable = std::get<MBC_BAND_ENABLE>(params),
                .cutoffFrequencyHz = cutOffFreqs[i].second,
                .attackTimeMs = additional[MBC_ADD_ATTACK_TIME],
                .releaseTimeMs = additional[MBC_ADD_RELEASE_TIME],
                .ratio = additional[MBC_ADD_RATIO],
                .thresholdDb = additional[MBC_ADD_THRESHOLD],
                .kneeWidthDb = additional[MBC_ADD_KNEE_WIDTH],
                .noiseGateThresholdDb = additional[MBC_ADD_NOISE_GATE_THRESHOLD],
                .expanderRatio = additional[MBC_ADD_EXPENDER_RATIO],
                .preGainDb = additional[MBC_ADD_PRE_GAIN],
                .postGainDb = additional[MBC_ADD_POST_GAIN]};
    }
}

class DynamicsProcessingTestMbcBandConfig
    : public ::testing::TestWithParam<TestParamsMbcBandConfig>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestMbcBandConfig()
        : DynamicsProcessingTestHelper(std::get<MBC_BAND_INSTANCE_NAME>(GetParam())),
          mStageInUse(std::get<MBC_BAND_STAGE_IN_USE>(GetParam())) {
        fillMbcBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::MbcBandConfig> mCfgs;
    const bool mStageInUse;
};

TEST_P(DynamicsProcessingTestMbcBandConfig, SetAndGetMbcBandConfig) {
    mEngineConfigPreset.mbcStage.inUse = mStageInUse;
    mEngineConfigPreset.mbcStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addMbcChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addMbcBandConfigs(mCfgs));
    SetAndGetDynamicsProcessingParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestMbcBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),  // channel count
                           testing::Bool(),             // enable
                           testing::ValuesIn(kBands),   // cut off frequencies
                           testing::Bool(),             // stage in use
                           testing::ValuesIn(kMbcBandConfigAdditionalParam)),  // Additional
        [](const auto& info) {
            auto descriptor = std::get<MBC_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::MbcBandConfig> cfgs;
            fillMbcBandConfig(cfgs, info.param);
            std::string mbcBands = ::android::internal::ToString(cfgs);
            std::string stageInUse = std::to_string(std::get<MBC_BAND_STAGE_IN_USE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_bands_" + mbcBands +
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
