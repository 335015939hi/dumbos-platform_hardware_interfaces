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

#define LOG_TAG "VtsHalEnvironmentalReverbTest"
#include <android-base/logging.h>
#include <audio_utils/power.h>
#include <system/audio.h>

#include "EffectHelper.h"

using namespace android;
using ParamTagValuePair =
        std::pair<aidl::android::hardware::audio::effect::EnvironmentalReverb::Tag,
                  std::vector<int>>;
using TagMinValuePair = std::pair<aidl::android::hardware::audio::effect::EnvironmentalReverb::Tag,
                                  aidl::android::hardware::audio::effect::EnvironmentalReverb>;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EnvironmentalReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

std::vector<ParamTagValuePair> reverbParams = {
        {EnvironmentalReverb::decayTimeMs, {1400, 2800, 4200, 5600}},
        {EnvironmentalReverb::roomLevelMb, {-3000, -2400, -1800, -1200, -600, 0}},
        {EnvironmentalReverb::decayHfRatioPm, {100, 600, 1100, 1600, 2000}},
        {EnvironmentalReverb::roomHfLevelMb, {-4000, -3200, -2400, -1600, -800, 0}},
        {EnvironmentalReverb::levelMb, {-6000, -4800, -3600, -2400, -1200, 0}},
};

std::vector<TagMinValuePair> minimumValueParams = {
        {EnvironmentalReverb::decayTimeMs,
         EnvironmentalReverb::make<EnvironmentalReverb::decayTimeMs>(0)},
        {EnvironmentalReverb::roomLevelMb,
         EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(-6000)}};

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing
 * performed in VtsAudioEffectTargetTest. Testing parameter range, assuming the parameter
 * supported by effect is in this range. This range is verified with IEffect.getDescriptor()
 * and range defined in the documentation, for any index supported value test expects
 * EX_NONE from IEffect.setParameter(), otherwise expects EX_ILLEGAL_ARGUMENT.
 */

class EnvironmentalReverbHelper : public EffectHelper {
  public:
    EnvironmentalReverbHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair) {
        std::tie(mFactory, mDescriptor) = pair;
    }

    void SetUpReverb() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                mFrameCount /* iFrameCount */, mFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownReverb() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        EnvironmentalReverb er =
                EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(kMaxRoomLevel);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::environmentalReverb>(er);
        return specific;
    }

    bool isParamValid(EnvironmentalReverb env) {
        return isParameterValid<EnvironmentalReverb, Range::environmentalReverb>(env, mDescriptor);
    }

    Parameter createParam(EnvironmentalReverb env) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::environmentalReverb>(env));
    }

    void setAndVerifyParam(binder_exception_t expected, EnvironmentalReverb env,
                           EnvironmentalReverb::Tag tag) {
        auto expectedParam = createParam(env);

        EXPECT_STATUS(expected, mEffect->setParameter(expectedParam)) << expectedParam.toString();

        if (expected == EX_NONE) {
            auto erId = EnvironmentalReverb::Id::make<EnvironmentalReverb::Id::commonTag>(
                    EnvironmentalReverb::Tag(tag));

            auto id = Parameter::Id::make<Parameter::Id::environmentalReverbTag>(erId);

            // get parameter
            Parameter getParam;
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectedParam, getParam) << "\nexpectedParam:" << expectedParam.toString()
                                               << "\ngetParam:" << getParam.toString();
        }
    }

    bool isAuxiliary() {
        return mDescriptor.common.flags.type ==
               aidl::android::hardware::audio::effect::Flags::Type::AUXILIARY;
    }

    float computeReverbOutputEnergy(std::vector<float> input, std::vector<float> output) {
        if (!isAuxiliary()) {
            // Extract auxiliary output
            for (size_t i = 0; i < output.size(); i++) {
                output[i] -= input[i];
            }
        }
        return (audio_utils_compute_energy_mono(output.data(), AUDIO_FORMAT_PCM_FLOAT,
                                                output.size()));
    }

    void generateSineWaveInput(std::vector<float>& input) {
        input.resize(kBufferSize);
        int frequency = 1000;
        size_t kSamplingFrequency = 44100;
        for (size_t i = 0; i < input.size(); i++) {
            input[i] = sin(2 * M_PI * frequency * i / kSamplingFrequency);
        }
    }

    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDurationMilliSec = 500;
    static constexpr int kBufferSize = kSamplingFrequency * kDurationMilliSec / 1000;
    static constexpr int kMaxRoomLevel = 0;
    static constexpr int kMinRoomLevel = -6000;
    static constexpr int kMinRoomHfLevel = -4000;
    static constexpr int kMinDecayTime = 0;
    static constexpr int kMinHfRatio = 100;
    static constexpr int kMinLevel = -6000;
    static constexpr int kMinDensity = 0;
    static constexpr int kMinDiffusion = 0;
    static constexpr int kMinDelay = 0;
    static constexpr bool kBypass = false;
    int mStereoChannelCount =
            getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO));
    int mFrameCount = kBufferSize / mStereoChannelCount;
    int mRoomLevel;
    int mRoomHfLevel;
    int mDecayTime;
    int mHfRatio;
    int mLevel;
    int mDelay;
    bool mBypass;
    int mDensity;
    int mDiffusion;

    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn ret;
    Descriptor mDescriptor;
};

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;

class EnvironmentalReverbDecayTimeTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDecayTimeTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDecayTime = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDecayTimeTest, SetAndGetDecayTime) {
    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::decayTimeMs>(mDecayTime);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::decayTimeMs));
}

class EnvironmentalReverbHfRatioTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbHfRatioTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mHfRatio = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbHfRatioTest, SetAndGetHfRatio) {
    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::decayHfRatioPm>(mHfRatio);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::decayHfRatioPm));
}

class EnvironmentalReverbRoomLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbRoomLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mRoomLevel = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbRoomLevelTest, SetAndGetRoomLevel) {
    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(mRoomLevel);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::roomLevelMb));
}

class EnvironmentalReverbRoomLevelTestDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbRoomLevelTestDataTest() : EnvironmentalReverbHelper(GetParam()) {}
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

class EnvironmentalReverbLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbLevelTest, SetAndGetLevel) {
    EnvironmentalReverb env = EnvironmentalReverb::make<EnvironmentalReverb::levelMb>(mLevel);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::levelMb));
}

class EnvironmentalReverbHfLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbHfLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mRoomHfLevel = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbHfLevelTest, SetAndGetHfLevel) {
    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::roomHfLevelMb>(mRoomHfLevel);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::roomHfLevelMb));
}

class EnvironmentalReverbDelayTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDelayTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDelay = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDelayTest, SetAndGetDelay) {
    EnvironmentalReverb env = EnvironmentalReverb::make<EnvironmentalReverb::delayMs>(mDelay);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::delayMs));
}

class EnvironmentalReverbBypassTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, bool>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbBypassTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mBypass = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbBypassTest, SetAndGetBypass) {
    EnvironmentalReverb env = EnvironmentalReverb::make<EnvironmentalReverb::bypass>(mBypass);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::bypass));
}

TEST_P(EnvironmentalReverbBypassTest, BypassDataTest) {
    EnvironmentalReverb env = EnvironmentalReverb::make<EnvironmentalReverb::bypass>(mBypass);
    if (!isParamValid(env)) {
        GTEST_SKIP() << "Skipping Test, invalid bypass param\n";
    }

    std::vector<float> input(kBufferSize);
    std::vector<float> output(kBufferSize);
    generateSineWaveInput(input);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(EX_NONE, env, EnvironmentalReverb::bypass));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &ret));
    float energy = computeReverbOutputEnergy(input, output);
    if (mBypass) {
        ASSERT_EQ(energy, 0);
    } else {
        ASSERT_NE(energy, 0);
    }
}

class EnvironmentalReverbDensityTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDensityTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDensity = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDensityTest, SetAndGetDensity) {
    EnvironmentalReverb env = EnvironmentalReverb::make<EnvironmentalReverb::densityPm>(mDensity);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::densityPm));
}

class EnvironmentalReverbDiffusionTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDiffusionTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDiffusion = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDiffusionTest, SetAndGetDiffusion) {
    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::diffusionPm>(mDiffusion);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(isParamValid(env) ? EX_NONE : EX_ILLEGAL_ARGUMENT,
                                              env, EnvironmentalReverb::diffusionPm));
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDecayTimeTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::decayTimeMs>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbDecayTimeTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string decayTime = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_decayTime" + decayTime;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDecayTimeTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbHfRatioTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::decayHfRatioPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbHfRatioTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string hfRatio = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_hfRatio" + hfRatio;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbHfRatioTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbRoomLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::roomLevelMb>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbRoomLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string roomLevel = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_roomLevel" + roomLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbRoomLevelTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::levelMb>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string level = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_Level" + level;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbLevelTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbHfLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::roomHfLevelMb>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbHfLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string hfLevel = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_hfLevel" + hfLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbHfLevelTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDelayTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::delayMs>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbDelayTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string delay = std::to_string(std::get<1>(info.param));
            std::string name = getPrefix(descriptor) + "_delay" + delay;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDelayTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbBypassTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::Bool()),
        [](const testing::TestParamInfo<EnvironmentalReverbBypassTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string bypass = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_bypass" + bypass;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbBypassTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDensityTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::densityPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbDensityTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string density = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_density" + density;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDensityTest);

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDiffusionTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::diffusionPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbDiffusionTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string diffusion = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_diffusion" + diffusion;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDiffusionTest);

class EnvironmentalDataTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, ParamTagValuePair>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalDataTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mParamPair = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }

    void assertEnergyIncreasingWithParameter() {
        std::vector<float> input(kBufferSize);
        generateSineWaveInput(input);
        createIncreasingParam();
        float baseEnergy = 0;
        for (EnvironmentalReverb env : mEnvParams) {
            std::vector<float> output(kBufferSize);
            // Skipping the further steps for unnsupported decay time values
            if (!isParamValid(env)) {
                continue;
            }
            ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(EX_NONE, env, mParamPair.first));
            ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &ret));
            float energy = computeReverbOutputEnergy(input, output);
            ASSERT_GT(energy, baseEnergy);
            baseEnergy = energy;
        }
    }

    void createIncreasingParam() {
        switch (mParamPair.first) {
            case EnvironmentalReverb::decayTimeMs:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::decayTimeMs>(
                                    paramValue));
                }
                break;
            case EnvironmentalReverb::decayHfRatioPm:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::decayHfRatioPm>(
                                    paramValue));
                }
                break;
            case EnvironmentalReverb::roomHfLevelMb:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::roomHfLevelMb>(
                                    paramValue));
                }
                break;
            case EnvironmentalReverb::levelMb:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::levelMb>(paramValue));
                }
                break;

            case EnvironmentalReverb::roomLevelMb:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(
                                    paramValue));
                }
                break;

            case EnvironmentalReverb::bypass:
                for (int paramValue : mParamPair.second) {
                    mEnvParams.push_back(
                            EnvironmentalReverb::make<EnvironmentalReverb::bypass>(paramValue));
                }
                break;

            default:
                GTEST_SKIP() << "Invalid parameter, skipping the test\n";
                break;
        }
    }
    ParamTagValuePair mParamPair;
    std::vector<EnvironmentalReverb> mEnvParams;
};

TEST_P(EnvironmentalDataTest, IncreasingParamValue) {
    assertEnergyIncreasingWithParameter();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(reverbParams)),
        [](const testing::TestParamInfo<EnvironmentalDataTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            auto val = std::get<1>(info.param);

            std::ostringstream oss;
            oss << static_cast<int>(val.first);
            std::string name = getPrefix(descriptor) + "Tag_" + oss.str();

            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalDataTest);

class MinimumParamValueTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, TagMinValuePair>>,
      public EnvironmentalReverbHelper {
  public:
    MinimumParamValueTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mParamPair = std::get<1>(GetParam());
    }
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }

    EnvironmentalReverb EnvParams;
    TagMinValuePair mParamPair;
};

TEST_P(MinimumParamValueTest, MinimumValueTest) {
    std::vector<float> input(kBufferSize);
    generateSineWaveInput(input);
    std::vector<float> output(kBufferSize);
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(EX_NONE, mParamPair.second, mParamPair.first));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &ret));
    float energy = computeReverbOutputEnergy(input, output);
    ASSERT_EQ(energy, 0);
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, MinimumParamValueTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(minimumValueParams)),
        [](const testing::TestParamInfo<MinimumParamValueTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            auto val = std::get<1>(info.param);

            std::ostringstream oss;
            oss << static_cast<int>(val.first);
            std::string name = getPrefix(descriptor) + "Tag_" + oss.str();
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MinimumParamValueTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
