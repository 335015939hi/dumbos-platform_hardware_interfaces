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

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EnvironmentalReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * This range is verified with IEffect.getDescriptor() and range defined in the documentation, for
 * any index supported value test expects EX_NONE from IEffect.setParameter(), otherwise expects
 * EX_ILLEGAL_ARGUMENT.
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

    void generateSineWaveInput(std::vector<float>& input) {
        int frequency = 1000;
        size_t kSamplingFrequency = 44100;
        for (size_t i = 0; i < input.size(); i++) {
            input[i] = sin(2 * M_PI * frequency * i / kSamplingFrequency);
        }
    }

    void assertIncreasingEnergy(const std::vector<EnvironmentalReverb>& envParams,
                                const EnvironmentalReverb::Tag& tag) {
        float baseEnergy = 0;
        std::vector<float> input(kBufferSize);
        generateSineWaveInput(input);

        for (EnvironmentalReverb env : envParams) {
            std::vector<float> output(kBufferSize);

            // Skipping the further steps for unnsupported decay time values
            if (!isParamValid(env)) {
                continue;
            }

            ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(EX_NONE, env, tag));
            ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &ret));
            float energy = audio_utils_compute_energy_mono(output.data(), AUDIO_FORMAT_PCM_FLOAT,
                                                           output.size());
            ASSERT_GT(energy, baseEnergy);
            baseEnergy = energy;
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

class EnvironmentalReverbDecayTimeTestDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDecayTimeTestDataTest() : EnvironmentalReverbHelper(GetParam()) {}
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDecayTimeTestDataTest, IncreasingDecayTime) {
    std::vector<int> increasingDecayTime = {100, 500, 1500, 2000};
    std::vector<EnvironmentalReverb> envParams;
    for (int decayTime : increasingDecayTime) {
        envParams.push_back(EnvironmentalReverb::make<EnvironmentalReverb::decayTimeMs>(decayTime));
    }
    assertIncreasingEnergy(envParams, EnvironmentalReverb::decayTimeMs);
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

class EnvironmentalReverbHfRatioTestDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbHfRatioTestDataTest() : EnvironmentalReverbHelper(GetParam()) {}
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbHfRatioTestDataTest, IncreasingHfRatio) {
    std::vector<int> increasingHfRatio = {100, 150, 200, 500, 600, 1000};
    std::vector<EnvironmentalReverb> envParams;
    for (int hFratio : increasingHfRatio) {
        envParams.push_back(
                EnvironmentalReverb::make<EnvironmentalReverb::decayHfRatioPm>(hFratio));
    }
    assertIncreasingEnergy(envParams, EnvironmentalReverb::decayHfRatioPm);
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

TEST_P(EnvironmentalReverbRoomLevelTestDataTest, IncreasingRoomLevel) {
    std::vector<int> increasingRoomLevel = {-3000, -2000, -1000, -500, 0};
    std::vector<EnvironmentalReverb> envParams;
    for (int roomLevel : increasingRoomLevel) {
        envParams.push_back(EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(roomLevel));
    }
    assertIncreasingEnergy(envParams, EnvironmentalReverb::roomLevelMb);
}

TEST_P(EnvironmentalReverbRoomLevelTestDataTest, MinimumRoomLevel) {
    std::vector<float> input(kBufferSize);
    std::vector<float> output(kBufferSize);

    EnvironmentalReverb env =
            EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(kMinRoomLevel);
    generateSineWaveInput(input);

    if (!isParamValid(env)) {
        GTEST_SKIP() << "Skipping the test, Base room level not supported\n";
    }
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParam(EX_NONE, env, EnvironmentalReverb::roomLevelMb));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &ret));
    float energy =
            audio_utils_compute_energy_mono(output.data(), AUDIO_FORMAT_PCM_FLOAT, output.size());
    ASSERT_EQ(energy, 0);
}

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

class EnvironmentalReverbLevelTestDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbLevelTestDataTest() : EnvironmentalReverbHelper(GetParam()) {}
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbLevelTestDataTest, IncreasingLevel) {
    std::vector<int> increasingLevel = {-6000, -3000, -2000, -1000, -500, 0};
    std::vector<EnvironmentalReverb> envParams;
    for (int level : increasingLevel) {
        envParams.push_back(EnvironmentalReverb::make<EnvironmentalReverb::levelMb>(level));
    }
    assertIncreasingEnergy(envParams, EnvironmentalReverb::levelMb);
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

class EnvironmentalReverbHfLevelTestDataTest
    : public ::testing::TestWithParam<std::pair<std::shared_ptr<IFactory>, Descriptor>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbHfLevelTestDataTest() : EnvironmentalReverbHelper(GetParam()) {}
    void SetUp() override { SetUpReverb(); }
    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbHfLevelTestDataTest, IncreasingHfLevel) {
    std::vector<int> increasingHfLevel = {-4000, -2000, -1000, -500, 0};

    std::vector<EnvironmentalReverb> envParams;
    for (int hfLevel : increasingHfLevel) {
        envParams.push_back(EnvironmentalReverb::make<EnvironmentalReverb::roomHfLevelMb>(hfLevel));
    }
    assertIncreasingEnergy(envParams, EnvironmentalReverb::roomHfLevelMb);
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

INSTANTIATE_TEST_SUITE_P(EnvironmentalReverbTest, EnvironmentalReverbDecayTimeTestDataTest,
                         testing::Values(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidEnvReverb())[2]),
                         [](const testing::TestParamInfo<
                                 EnvironmentalReverbDecayTimeTestDataTest::ParamType>& info) {
                             auto descriptor = info.param;
                             return getPrefix(descriptor.second);
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDecayTimeTestDataTest);

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
        EnvironmentalReverbTest, EnvironmentalReverbHfRatioTestDataTest,
        testing::Values(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidEnvReverb())[2]),
        [](const testing::TestParamInfo<EnvironmentalReverbHfRatioTestDataTest::ParamType>& info) {
            auto descriptor = info.param;
            return getPrefix(descriptor.second);
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbHfRatioTestDataTest);

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

INSTANTIATE_TEST_SUITE_P(EnvironmentalReverbTest, EnvironmentalReverbRoomLevelTestDataTest,
                         testing::Values(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidEnvReverb())[2]),
                         [](const testing::TestParamInfo<
                                 EnvironmentalReverbRoomLevelTestDataTest::ParamType>& info) {
                             auto descriptor = info.param;
                             return getPrefix(descriptor.second);
                         });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbRoomLevelTestDataTest);

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
        EnvironmentalReverbTest, EnvironmentalReverbLevelTestDataTest,
        testing::Values(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidEnvReverb())[2]),
        [](const testing::TestParamInfo<EnvironmentalReverbLevelTestDataTest::ParamType>& info) {
            auto descriptor = info.param;
            return getPrefix(descriptor.second);
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbLevelTestDataTest);

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
        EnvironmentalReverbTest, EnvironmentalReverbHfLevelTestDataTest,
        testing::Values(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidEnvReverb())[2]),
        [](const testing::TestParamInfo<EnvironmentalReverbHfLevelTestDataTest::ParamType>& info) {
            auto descriptor = info.param;
            return getPrefix(descriptor.second);
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbHfLevelTestDataTest);

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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
