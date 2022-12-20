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
    DynamicsProcessingTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair) {
        std::tie(mFactory, mDescriptor) = pair;
    }

    // setup
    void SetUpDynamicsProcessingEffect() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon();
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
    bool isParamValid(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dp);
    bool isParamEqual(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dpRef,
                      const DynamicsProcessing& dpTest);
    bool isEngineConfigValid(const DynamicsProcessing::EngineArchitecture& cfg);
    bool isLimiterConfigValid(const DynamicsProcessing::LimiterConfig& cfg);
    bool isPreEqBandChannelConfigValid(const DynamicsProcessing::BandChannelConfig& cfg);
    bool isPostEqBandChannelConfigValid(const DynamicsProcessing::BandChannelConfig& cfg);
    bool isMbcBandChannelConfigValid(const DynamicsProcessing::BandChannelConfig& cfg);
    bool isEngineConfigEqual(const DynamicsProcessing::EngineArchitecture& ref,
                             const DynamicsProcessing::EngineArchitecture& test);
    bool isLimiterConfigEqual(const DynamicsProcessing::LimiterConfig& ref,
                              const DynamicsProcessing::LimiterConfig& test);
    bool isBandChannelConfigEqual(const DynamicsProcessing::BandChannelConfig& ref,
                                  const DynamicsProcessing::BandChannelConfig& test);

    // get set params and validate
    void SetAndGetDynamicsProcessingParameters();

    // enqueue test parameters
    void addEngineConfig(DynamicsProcessing::EngineArchitecture cfg);
    void addInputGain(float inputGaindB);
    void addLimiterConfig(DynamicsProcessing::LimiterConfig cfg);
    void addPreEqBandChannelConfig(DynamicsProcessing::BandChannelConfig cfg);
    void addPostEqBandChannelConfig(DynamicsProcessing::BandChannelConfig cfg);
    void addMbcBandChannelConfig(DynamicsProcessing::BandChannelConfig cfg);

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
    std::vector<std::tuple<DynamicsProcessing::Tag, DynamicsProcessing, DynamicsProcessing>> mTags;
    void CleanUp() { mTags.clear(); }
};

bool DynamicsProcessingTestHelper::isParamValid(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dp) {
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigValid(dp.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::inputGainDb: {
            return true;
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigValid(dp.get<DynamicsProcessing::limiter>());
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
        default:
            return false;
    }
    return false;
}

bool DynamicsProcessingTestHelper::isParamEqual(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dpRef,
                                                const DynamicsProcessing& dpTest) {
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigEqual(dpRef.get<DynamicsProcessing::engineArchitecture>(),
                                       dpTest.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::inputGainDb: {
            return dpRef.get<DynamicsProcessing::inputGainDb>() ==
                   dpTest.get<DynamicsProcessing::inputGainDb>();
        }
        case DynamicsProcessing::limiter: {
            return isLimiterConfigEqual(dpRef.get<DynamicsProcessing::limiter>(),
                                        dpTest.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::preEq: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::preEq>(),
                                            dpTest.get<DynamicsProcessing::preEq>());
        }
        case DynamicsProcessing::postEq: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::postEq>(),
                                            dpTest.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isBandChannelConfigEqual(dpRef.get<DynamicsProcessing::mbc>(),
                                            dpTest.get<DynamicsProcessing::mbc>());
        }
        default:
            return false;
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

bool DynamicsProcessingTestHelper::isLimiterConfigValid(
        const DynamicsProcessing::LimiterConfig& cfg) {
    if (cfg.inUse != mEngineConfig.limiterInUse) return false;
    if (cfg.inUse) {
        if (cfg.channel < 0 || cfg.channel >= 2) return false;
        if (cfg.attackTimeMs < 0) return false;
        if (cfg.releaseTimeMs < 0) return false;
        if (cfg.ratio < 0) return false;
        if (cfg.thresholdDb > 0) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isPreEqBandChannelConfigValid(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    if (cfg.enablement.inUse != mEngineConfig.preEqBand.inUse) return false;
    if (cfg.enablement.inUse) {
        if (cfg.channel < 0 || cfg.channel >= 2) return false;
        if (cfg.enablement.bandCount != mEngineConfig.preEqBand.bandCount) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isPostEqBandChannelConfigValid(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    if (cfg.enablement.inUse != mEngineConfig.postEqBand.inUse) return false;
    if (cfg.enablement.inUse) {
        if (cfg.channel < 0 || cfg.channel >= 2) return false;
        if (cfg.enablement.bandCount != mEngineConfig.postEqBand.bandCount) return false;
    }
    return true;
}

bool DynamicsProcessingTestHelper::isMbcBandChannelConfigValid(
        const DynamicsProcessing::BandChannelConfig& cfg) {
    if (cfg.enablement.inUse != mEngineConfig.mbcBand.inUse) return false;
    if (cfg.enablement.inUse) {
        if (cfg.channel < 0 || cfg.channel >= 2) return false;
        if (cfg.enablement.bandCount != mEngineConfig.mbcBand.bandCount) return false;
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

bool DynamicsProcessingTestHelper::isLimiterConfigEqual(
        const DynamicsProcessing::LimiterConfig& ref,
        const DynamicsProcessing::LimiterConfig& test) {
    if (ref.inUse != test.inUse) return false;
    if (ref.inUse) return ref == test;
    return true;
}

bool DynamicsProcessingTestHelper::isBandChannelConfigEqual(
        const DynamicsProcessing::BandChannelConfig& ref,
        const DynamicsProcessing::BandChannelConfig& test) {
    if (ref.enablement.inUse != test.enablement.inUse) return false;
    if (ref.enablement.inUse) return ref == test;
    return true;
}

void DynamicsProcessingTestHelper::SetAndGetDynamicsProcessingParameters() {
    for (auto& it : mTags) {
        auto& tag = std::get<0>(it);
        auto& dp = std::get<1>(it);
        auto& dpOut = std::get<2>(it);

        // validate parameter
        const bool valid = isParamValid(tag, dp);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::dynamicsProcessing>(dp);
        expectParam.set<Parameter::specific>(specific);
        EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

        // only get if parameter in range and set success
        if (expected == EX_NONE) {
            Parameter outParam;
            Parameter::Specific outSpecific;
            specific.set<Parameter::Specific::dynamicsProcessing>(dpOut);
            outParam.set<Parameter::specific>(outSpecific);

            Parameter::Id id;
            DynamicsProcessing::Id dpId;
            dpId.set<DynamicsProcessing::Id::commonTag>(tag);
            id.set<Parameter::Id::dynamicsProcessingTag>(dpId);
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &outParam));
            Parameter::Specific specificTest = outParam.get<Parameter::specific>();
            EXPECT_TRUE(isParamEqual(tag, dp,
                                     specificTest.get<Parameter::Specific::dynamicsProcessing>()));
        }
    }
}

void DynamicsProcessingTestHelper::addEngineConfig(DynamicsProcessing::EngineArchitecture cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::engineArchitecture>(cfg);
    DynamicsProcessing dpUnused;
    mTags.push_back(std::make_tuple(DynamicsProcessing::engineArchitecture, dp, dpUnused));
}

void DynamicsProcessingTestHelper::addInputGain(float inputGaindB) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::inputGainDb>(inputGaindB);
    DynamicsProcessing dpUnused;
    mTags.push_back(std::make_tuple(DynamicsProcessing::inputGainDb, dp, dpUnused));
}

void DynamicsProcessingTestHelper::addLimiterConfig(DynamicsProcessing::LimiterConfig cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::limiter>(cfg);
    DynamicsProcessing dpUnused;
    mTags.push_back(std::make_tuple(DynamicsProcessing::limiter, dp, dpUnused));
}

void DynamicsProcessingTestHelper::addPreEqBandChannelConfig(
        DynamicsProcessing::BandChannelConfig cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEq>(cfg);
    DynamicsProcessing dpOut;
    DynamicsProcessing::BandChannelConfig cfgOut;
    cfgOut.channel = cfg.channel;
    dpOut.set<DynamicsProcessing::preEq>(cfgOut);
    mTags.push_back(std::make_tuple(DynamicsProcessing::preEq, dp, dpOut));
}

void DynamicsProcessingTestHelper::addPostEqBandChannelConfig(
        DynamicsProcessing::BandChannelConfig cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEq>(cfg);
    DynamicsProcessing dpOut;
    DynamicsProcessing::BandChannelConfig cfgOut;
    cfgOut.channel = cfg.channel;
    dpOut.set<DynamicsProcessing::postEq>(cfgOut);
    mTags.push_back(std::make_tuple(DynamicsProcessing::postEq, dp, dpOut));
}

void DynamicsProcessingTestHelper::addMbcBandChannelConfig(
        DynamicsProcessing::BandChannelConfig cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbc>(cfg);
    DynamicsProcessing dpOut;
    DynamicsProcessing::BandChannelConfig cfgOut;
    cfgOut.channel = cfg.channel;
    dpOut.set<DynamicsProcessing::mbc>(cfgOut);
    mTags.push_back(std::make_tuple(DynamicsProcessing::mbc, dp, dpOut));
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
                           testing::Values(-1, 0, 1, 2),  // channel count
                           testing::Bool(),               // enable
                           testing::Bool(),               // inuse
                           testing::Values(3),            // link group
                           testing::Values(-1, 1),        // attack time ms
                           testing::Values(-60, 60),      // release time ms
                           testing::Values(-2.5, 2.5),    // ratio
                           testing::Values(-2, 2),        // threshold
                           testing::Values(3.14f),        // post gain
                           testing::Bool()),              // engine limiter enable
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
