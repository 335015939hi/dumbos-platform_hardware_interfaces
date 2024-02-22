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

#define LOG_TAG "VtsHalVirtualizerTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVirtualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Virtualizer;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

class VirtualizerHelper : public EffectHelper {
  public:
    void SetUpVirtualizer() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        initFrameCount();
        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, mInputFrameCount /* iFrameCount */,
                mInputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownVirtualizer() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter::Specific getDefaultParamSpecific() {
        Virtualizer vr = Virtualizer::make<Virtualizer::strengthPm>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::virtualizer>(vr);
        return specific;
    }

    Parameter createVirtualizerParam(int param) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::virtualizer>(
                        Virtualizer::make<Virtualizer::strengthPm>(param)));
    }

    void initFrameCount() {
        mInputFrameCount = kBufferSize / kChannelCount;
        mOutputFrameCount = kBufferSize / kChannelCount;
    }

    bool isStrengthValid(int level) {
        auto vir = Virtualizer::make<Virtualizer::strengthPm>(level);
        return isParameterValid<Virtualizer, Range::virtualizer>(vir, mDescriptor);
    }

    void setAndVerifyParameters(int param, binder_exception_t expected) {
        auto expectedParam = createVirtualizerParam(param);
        EXPECT_STATUS(expected, mEffect->setParameter(expectedParam)) << expectedParam.toString();

        if (expected == EX_NONE) {
            Virtualizer::Id vrlId =
                    Virtualizer::Id::make<Virtualizer::Id::commonTag>(Virtualizer::strengthPm);

            auto id = Parameter::Id::make<Parameter::Id::virtualizerTag>(vrlId);
            // get parameter
            Parameter getParam;
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectedParam, getParam) << "\nexpectedParam:" << expectedParam.toString()
                                               << "\ngetParam:" << getParam.toString();
        }
    }

    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDefaultChannelLayout = AudioChannelLayout::LAYOUT_STEREO;
    static constexpr int kDurationMilliSec = 2000;
    static constexpr int kBufferSize = kSamplingFrequency * kDurationMilliSec / 1000;
    int kChannelCount = getChannelCount(
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(kDefaultChannelLayout));
    long mInputFrameCount;
    long mOutputFrameCount;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    Descriptor mDescriptor;
};

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_STRENGTH };
using VirtualizerParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>;

/*
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * Parameter should be within the valid range defined in the documentation,
 * for any supported value test expects EX_NONE from IEffect.setParameter(),
 * otherwise expect EX_ILLEGAL_ARGUMENT.
 */

class VirtualizerParamTest : public ::testing::TestWithParam<VirtualizerParamTestParam>,
                             public VirtualizerHelper {
  public:
    VirtualizerParamTest() : mParamStrength(std::get<PARAM_STRENGTH>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpVirtualizer()); }
    void TearDown() override { TearDownVirtualizer(); }

    int mParamStrength = 0;
};

TEST_P(VirtualizerParamTest, SetAndGetStrength) {
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(
            mParamStrength, isStrengthValid(mParamStrength) ? EX_NONE : EX_ILLEGAL_ARGUMENT));
}

using VirtualizerProcessTestParam = std::pair<std::shared_ptr<IFactory>, Descriptor>;
class VirtualizerProcessTest : public ::testing::TestWithParam<VirtualizerProcessTestParam>,
                               public VirtualizerHelper {
  public:
    VirtualizerProcessTest() { std::tie(mFactory, mDescriptor) = GetParam(); }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpVirtualizer()); }
    void TearDown() override { TearDownVirtualizer(); }

    void generateInput(std::vector<float>& buffer) {
        int frequency = 100;
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = sin(2 * M_PI * frequency * i / kSamplingFrequency);
        }
    }

    float getCrossCorrelationStereo(const std::vector<float>& buffer) {
        std::vector<float> left(kBufferSize / kChannelCount);
        std::vector<float> right(kBufferSize / kChannelCount);

        for (size_t i = 0, j = 0; i < buffer.size(); i += kChannelCount, j++) {
            left[j] = buffer[i];
            right[j] = buffer[i + 1];
        }

        float sum = 0;
        for (size_t i = 0; i < buffer.size() / kChannelCount; i++) {
            sum += left[i] * right[i];
        }

        return sum / (buffer.size() / kChannelCount);
    }
};

TEST_P(VirtualizerProcessTest, IncreasingStrength) {
    std::vector<float> input(kBufferSize);
    std::vector<float> output(kBufferSize);
    std::vector<int> strengths = {500, 750, 1000};

    generateInput(input);

    if (!isStrengthValid(0 /*Base strength*/)) {
        GTEST_SKIP() << "Invalid strength value, skipping the test\n";
    }
    setAndVerifyParameters(0 /*Base strength*/, EX_NONE);
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(input, output, mEffect, &mOpenEffectReturn));
    float baseCrossCorelation = getCrossCorrelationStereo(output);

    for (int strength : strengths) {
        // Skipping the further steps for unnsupported Strength values
        if (!isStrengthValid(strength)) {
            continue;
        }
        setAndVerifyParameters(strength, EX_NONE);
        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(input, output, mEffect, &mOpenEffectReturn));
        float diff = getCrossCorrelationStereo(output);
        // Cross-Correlation close to 1 indicates that samples are identical
        // High strength value, low Cross-Correlation
        ASSERT_LT(diff, baseCrossCorelation);
        baseCrossCorelation = diff;
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        VirtualizerTest, VirtualizerParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidVirtualizer())),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  Virtualizer, int, Range::virtualizer, Virtualizer::strengthPm>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<VirtualizerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string strength = std::to_string(std::get<PARAM_STRENGTH>(info.param));
            std::string name = getPrefix(descriptor) + "_strength" + strength;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VirtualizerParamTest);

INSTANTIATE_TEST_SUITE_P(VirtualizerTest, VirtualizerProcessTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidVirtualizer())),
                         [](const testing::TestParamInfo<VirtualizerProcessTest::ParamType>& info) {
                             auto descriptor = info.param;

                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VirtualizerProcessTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
