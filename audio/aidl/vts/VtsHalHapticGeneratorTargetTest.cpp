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

#include <map>
#include <utility>
#include <vector>

#define LOG_TAG "VtsHalHapticGeneratorTargetTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>
#include <audio_utils/power.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidHapticGenerator;
using aidl::android::hardware::audio::effect::HapticGenerator;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_HAPTIC_SCALE_ID,
    PARAM_HAPTIC_SCALE_VIBRATOR_SCALE,
    PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY,
    PARAM_VIBRATION_INFORMATION_Q_FACTOR,
    PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE,
};
using HapticGeneratorParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int,
                   HapticGenerator::VibratorScale, float, float, float>;

/*
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * Parameter should be within the valid range defined in the documentation,
 * for any supported value test expects EX_NONE from IEffect.setParameter(),
 * otherwise expect EX_ILLEGAL_ARGUMENT.
 */

// TODO : Update the test values once range/capability is updated by implementation
const int MIN_ID = std::numeric_limits<int>::min();
const int MAX_ID = std::numeric_limits<int>::max();
const float MIN_FLOAT = std::numeric_limits<float>::min();
const float MAX_FLOAT = std::numeric_limits<float>::max();

const std::vector<int> kHapticScaleIdValues = {MIN_ID, 0, MAX_ID};
const std::vector<HapticGenerator::VibratorScale> kVibratorScaleValues = {
        ndk::enum_range<HapticGenerator::VibratorScale>().begin(),
        ndk::enum_range<HapticGenerator::VibratorScale>().end()};

const std::vector<float> kResonantFrequencyValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kQFactorValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kMaxAmplitude = {MIN_FLOAT, 100, MAX_FLOAT};

static const std::vector<int32_t> kHapticOutputLayouts = {AudioChannelLayout::CHANNEL_HAPTIC_A,
                                                          AudioChannelLayout::CHANNEL_HAPTIC_B,
                                                          AudioChannelLayout::LAYOUT_HAPTIC_AB};

class HapticGeneratorHelper : public EffectHelper {
  public:
    void SetUpHapticGenerator(int32_t outputLayout = AudioChannelLayout::CHANNEL_HAPTIC_A) {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        AudioChannelLayout inputchannelLayout =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                        AudioChannelLayout::LAYOUT_MONO);
        AudioChannelLayout outputchannelLayout =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(outputLayout);

        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */,
                inputchannelLayout, outputchannelLayout);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownHapticGenerator() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        ret = IEffect::OpenEffectReturn{};
    }

    void setAndVerifyScaleParameter(int scaleId, HapticGenerator::VibratorScale scale) {
        std::vector<HapticGenerator::HapticScale> hapticScales = {{.id = scaleId, .scale = scale}};
        auto expectParam = Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::hapticGenerator>(
                        HapticGenerator::make<HapticGenerator::hapticScales>(hapticScales)));
        EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

        auto hapticId = HapticGenerator::Id::make<HapticGenerator::Id::commonTag>(
                HapticGenerator::Tag(HapticGenerator::hapticScales));
        auto id = Parameter::Id::make<Parameter::Id::hapticGeneratorTag>(hapticId);
        // get parameter
        Parameter getParam;
        // If the set is successful, get param should match
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        EXPECT_EQ(expectParam, getParam) << "\nexpectedParam:" << expectParam.toString()
                                         << "\ngetParam:" << getParam.toString();
    }

    void setAndVerifyVibratorParameter(float resonantFrequencyHz, float qFactor,
                                       float maxAmplitude) {
        HapticGenerator::VibratorInformation vibrationInfo = {
                .resonantFrequencyHz = resonantFrequencyHz,
                .qFactor = qFactor,
                .maxAmplitude = maxAmplitude};
        auto expectParam = Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::hapticGenerator>(
                        HapticGenerator::make<HapticGenerator::vibratorInfo>(vibrationInfo)));
        EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

        auto hapticId = HapticGenerator::Id::make<HapticGenerator::Id::commonTag>(
                HapticGenerator::Tag(HapticGenerator::vibratorInfo));
        auto id = Parameter::Id::make<Parameter::Id::hapticGeneratorTag>(hapticId);
        // get parameter
        Parameter getParam;
        // If the set is successful, get param should match
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        EXPECT_EQ(expectParam, getParam) << "\nexpectedParam:" << expectParam.toString()
                                         << "\ngetParam:" << getParam.toString();
    }

    static const long kInputFrameCount = 10000;
    static const long kHapticFrameCount = kInputFrameCount;
    static const long kOutputFrameCount = kInputFrameCount + kHapticFrameCount;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn ret;
};

class HapticGeneratorParamTest : public ::testing::TestWithParam<HapticGeneratorParamTestParam>,
                                 public HapticGeneratorHelper {
  public:
    HapticGeneratorParamTest()
        : mParamHapticScaleId(std::get<PARAM_HAPTIC_SCALE_ID>(GetParam())),
          mParamVibratorScale(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(GetParam())),
          mParamResonantFrequency(
                  std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(GetParam())),
          mParamQFactor(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(GetParam())),
          mParamMaxAmplitude(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpHapticGenerator()); }
    void TearDown() override { TearDownHapticGenerator(); }

    int mParamHapticScaleId = 0;
    HapticGenerator::VibratorScale mParamVibratorScale = HapticGenerator::VibratorScale::MUTE;
    float mParamResonantFrequency = 0;
    float mParamQFactor = 0;
    float mParamMaxAmplitude = 0;
};

TEST_P(HapticGeneratorParamTest, SetAndGetHapticScale) {
    ASSERT_NO_FATAL_FAILURE(setAndVerifyScaleParameter(mParamHapticScaleId, mParamVibratorScale));
}

TEST_P(HapticGeneratorParamTest, SetAndGetMultipleHapticScales) {
    ASSERT_NO_FATAL_FAILURE(setAndVerifyScaleParameter(mParamHapticScaleId, mParamVibratorScale));
    ASSERT_NO_FATAL_FAILURE(setAndVerifyScaleParameter(mParamHapticScaleId, mParamVibratorScale));
}

TEST_P(HapticGeneratorParamTest, SetAndGetVibratorInformation) {
    ASSERT_NO_FATAL_FAILURE(setAndVerifyVibratorParameter(mParamResonantFrequency, mParamQFactor,
                                                          mParamMaxAmplitude));
}

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorValidTest, HapticGeneratorParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::ValuesIn(kHapticScaleIdValues),
                           testing::ValuesIn(kVibratorScaleValues),
                           testing::ValuesIn(kResonantFrequencyValues),
                           testing::ValuesIn(kQFactorValues), testing::ValuesIn(kMaxAmplitude)),
        [](const testing::TestParamInfo<HapticGeneratorParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string hapticScaleID = std::to_string(std::get<PARAM_HAPTIC_SCALE_ID>(info.param));
            std::string hapticScaleVibScale = std::to_string(
                    static_cast<int>(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(info.param)));
            std::string resonantFrequency = std::to_string(
                    std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(info.param));
            std::string qFactor =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(info.param));
            std::string maxAmplitude =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(info.param));
            std::string name = getPrefix(descriptor) + "_hapticScaleId" + hapticScaleID +
                               "_hapticScaleVibScale" + hapticScaleVibScale + "_resonantFrequency" +
                               resonantFrequency + "_qFactor" + qFactor + "_maxAmplitude" +
                               maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorInvalidTest, HapticGeneratorParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::Values(MIN_ID),
                           testing::Values(HapticGenerator::VibratorScale::NONE),
                           testing::Values(MIN_FLOAT), testing::Values(MIN_FLOAT),
                           testing::Values(MIN_FLOAT)),
        [](const testing::TestParamInfo<HapticGeneratorParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string hapticScaleID = std::to_string(std::get<PARAM_HAPTIC_SCALE_ID>(info.param));
            std::string hapticScaleVibScale = std::to_string(
                    static_cast<int>(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(info.param)));
            std::string resonantFrequency = std::to_string(
                    std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(info.param));
            std::string qFactor =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(info.param));
            std::string maxAmplitude =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_hapticScaleId" +
                               hapticScaleID + "_hapticScaleVibScale" + hapticScaleVibScale +
                               "_resonantFrequency" + resonantFrequency + "_qFactor" + qFactor +
                               "_maxAmplitude" + maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorParamTest);

enum DataTestParam { DATA_INSTANCE_NAME, DATA_LAYOUT };
using HapticGeneratorDataTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t>;

class HapticGeneratorDataTest : public ::testing::TestWithParam<HapticGeneratorDataTestParam>,
                                public HapticGeneratorHelper {
  public:
    HapticGeneratorDataTest() : mOutputLayout(std::get<DATA_LAYOUT>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<DATA_INSTANCE_NAME>(GetParam());
        mHapticChannelCount = getChannelCount(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mOutputLayout),
                AudioChannelLayout::LAYOUT_HAPTIC_AB);
        mHapticOutputBufferSize = kHapticFrameCount * mHapticChannelCount;
        mOutputBufferSize = mHapticOutputBufferSize + kInputBufferSize;
        generateInput();
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpHapticGenerator(mOutputLayout)); }
    void TearDown() override { TearDownHapticGenerator(); }

    void generateInput() {
        int frequency = 1000;
        for (size_t i = 0; i < kInputBufferSize; i++) {
            mInput.push_back(sin(2 * M_PI * frequency * i / 44100));
        }
    }

    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDefaultScaleID = 0;
    static constexpr float kDefaultMaxAmp = 1;
    static constexpr float kDefaultResonantFrequency = 150;
    static constexpr float kDefaultQfactor = 8;
    static constexpr HapticGenerator::VibratorScale kDefaultScale =
            HapticGenerator::VibratorScale::NONE;
    int kInputChannelCount =
            getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_MONO));
    size_t kInputBufferSize = kInputFrameCount * kInputChannelCount;

    size_t mOutputBufferSize;
    size_t mHapticOutputBufferSize;
    int32_t mOutputLayout;
    int mHapticChannelCount;
    std::vector<float> mInput;
};

TEST_P(HapticGeneratorDataTest, IncreaseingScaleTest) {
    std::vector<float> output(mOutputBufferSize);
    ASSERT_NO_FATAL_FAILURE(
            setAndVerifyScaleParameter(kDefaultScaleID, HapticGenerator::VibratorScale::MUTE));
    ASSERT_NO_FATAL_FAILURE(setAndVerifyVibratorParameter(kDefaultResonantFrequency,
                                                          kDefaultQfactor, kDefaultMaxAmp));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &ret));

    float baseHapticOutputEnergy = audio_utils_compute_energy_mono(
            output.data() + kInputBufferSize, AUDIO_FORMAT_PCM_FLOAT, mHapticOutputBufferSize);

    for (size_t i = 1; i < kVibratorScaleValues.size(); i++) {
        ASSERT_NO_FATAL_FAILURE(
                setAndVerifyScaleParameter(kDefaultScaleID, kVibratorScaleValues[i]));

        ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &ret));
        float hapticOutputEnergy = audio_utils_compute_energy_mono(
                output.data() + kInputBufferSize, AUDIO_FORMAT_PCM_FLOAT, mHapticOutputBufferSize);
        ASSERT_GT(hapticOutputEnergy, baseHapticOutputEnergy);
        baseHapticOutputEnergy = hapticOutputEnergy;
    }
}

TEST_P(HapticGeneratorDataTest, DecreasingMaxAmplitudeTest) {
    std::vector<float> output(mOutputBufferSize);
    std::vector<float> descreasingAmplitudeValues = {0.75, 0.5, 0.25, 0.1};

    ASSERT_NO_FATAL_FAILURE(setAndVerifyScaleParameter(kDefaultScaleID, kDefaultScale));
    ASSERT_NO_FATAL_FAILURE(setAndVerifyVibratorParameter(kDefaultResonantFrequency,
                                                          kDefaultQfactor, kDefaultMaxAmp));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &ret));
    float baseHapticOutputEnergy = audio_utils_compute_energy_mono(
            output.data() + kInputBufferSize, AUDIO_FORMAT_PCM_FLOAT, mHapticOutputBufferSize);
    for (float amplitude : descreasingAmplitudeValues) {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyVibratorParameter(kDefaultResonantFrequency,
                                                              kDefaultQfactor, amplitude));
        ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &ret));
        float hapticOutputEnergy = audio_utils_compute_energy_mono(
                output.data() + kInputBufferSize, AUDIO_FORMAT_PCM_FLOAT, mHapticOutputBufferSize);
        ASSERT_LT(hapticOutputEnergy, baseHapticOutputEnergy);
        baseHapticOutputEnergy = hapticOutputEnergy;
    }
}

INSTANTIATE_TEST_SUITE_P(
        DataTest, HapticGeneratorDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::ValuesIn(kHapticOutputLayouts)),
        [](const testing::TestParamInfo<HapticGeneratorDataTest::ParamType>& info) {
            auto descriptor = std::get<DATA_INSTANCE_NAME>(info.param).second;
            std::string layout = std::to_string(std::get<DATA_LAYOUT>(info.param));
            std::string name = getPrefix(descriptor) + "_layout_" + layout;
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
