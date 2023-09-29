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

#include <string>

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalLoudnessEnhancerTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidLoudnessEnhancer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::LoudnessEnhancer;
using aidl::android::hardware::audio::effect::Parameter;

static constexpr float kMaxAudioSample = 1;
static constexpr int kZeroGain = 0;
static constexpr int kMaxGain = std::numeric_limits<int>::max();
static constexpr int kMinGain = std::numeric_limits<int>::min();
static constexpr float kAbsError = 0.0001;

// Every int 32 bit value is a valid gain, so testing the corner cases and one regular value.
// TODO : Update the test values once range/capability is updated by implementation.
static const std::vector<int> kGainMbValues = {kMinGain, -100, -50, kZeroGain, 50, 100, kMaxGain};

class LoudnessEnhancerEffectHelper : public EffectHelper {
  public:
    void SetUpLoudnessEnhancer() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownLoudnessEnhancer() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        LoudnessEnhancer le = LoudnessEnhancer::make<LoudnessEnhancer::gainMb>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::loudnessEnhancer>(le);
        return specific;
    }

    Parameter createLoudnessParam(int gainMb) {
        LoudnessEnhancer le;
        le.set<LoudnessEnhancer::gainMb>(gainMb);
        Parameter param;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::loudnessEnhancer>(le);
        param.set<Parameter::specific>(specific);
        return param;
    }

    bool isGainValid(int gainMb) {
        LoudnessEnhancer le;
        le.set<LoudnessEnhancer::gainMb>(gainMb);
        return isParameterValid<LoudnessEnhancer, Range::loudnessEnhancer>(le, mDescriptor);
    }

    binder_exception_t SetParameters(int gain) {
        bool valid = isGainValid(gain);
        binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        auto param = createLoudnessParam(gain);

        EXPECT_STATUS(expected, mEffect->setParameter(param)) << param.toString();
        return expected;
    }

    void validateParameters(int gain) {
        // get parameter
        LoudnessEnhancer::Id leId;
        Parameter getParam;
        Parameter::Id id;

        LoudnessEnhancer::Tag tag(LoudnessEnhancer::gainMb);
        leId.set<LoudnessEnhancer::Id::commonTag>(tag);
        id.set<Parameter::Id::loudnessEnhancerTag>(leId);
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        auto expectedParam = createLoudnessParam(gain);
        EXPECT_EQ(expectedParam, getParam)
                << "\nexpect:" << expectedParam.toString() << "\ngetParam:" << getParam.toString();
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
};

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_GAIN_MB };
using LoudnessEnhancerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>;

class LoudnessEnhancerParamTest : public ::testing::TestWithParam<LoudnessEnhancerParamTestParam>,
                                  public LoudnessEnhancerEffectHelper {
  public:
    LoudnessEnhancerParamTest() : mParamGainMb(std::get<PARAM_GAIN_MB>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override { SetUpLoudnessEnhancer(); }
    void TearDown() override { TearDownLoudnessEnhancer(); }
    int mParamGainMb = 0;
};

TEST_P(LoudnessEnhancerParamTest, SetAndGetGainMb) {
    binder_exception_t expected = SetParameters(mParamGainMb);
    if (expected == EX_NONE) validateParameters(mParamGainMb);
}

using LoudnessEnhancerDataTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>>;

class LoudnessEnhancerDataTest : public ::testing::TestWithParam<LoudnessEnhancerDataTestParam>,
                                 public LoudnessEnhancerEffectHelper {
  public:
    LoudnessEnhancerDataTest() {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
        generateInputBuffer();
        mOutputBuffer.resize(kBufferSize);
    }

    void SetUp() override {
        SetUpLoudnessEnhancer();

        // Creating AidlMessageQueues
        mStatusMQ = std::make_unique<EffectHelper::StatusMQ>(mOpenEffectReturn.statusMQ);
        mInputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn.inputDataMQ);
        mOutputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn.outputDataMQ);
    }

    void TearDown() override { TearDownLoudnessEnhancer(); }

    // Fill inputBuffer with random values between -kMaxAudioSample to kMaxAudioSample
    void generateInputBuffer() {
        for (size_t i = 0; i < kBufferSize; i++) {
            mInputBuffer.push_back(((static_cast<float>(std::rand()) / RAND_MAX) * 2 - 1) *
                                   kMaxAudioSample);
        }
    }

    void process() {
        // Check AidlMessageQueues are not null
        ASSERT_TRUE(mStatusMQ->isValid());
        ASSERT_TRUE(mInputMQ->isValid());
        ASSERT_TRUE(mOutputMQ->isValid());

        // Enabling the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
        ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

        // Write from buffer to message queues and calling process
        EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(mStatusMQ, mInputMQ, mInputBuffer));

        // Read the updated message queues into buffer
        EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(mStatusMQ, 1, mOutputMQ,
                                                          mOutputBuffer.size(), mOutputBuffer));

        // Disable the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    }

    void assertGreaterGain(std::vector<float> first, std::vector<float> second) {
        for (size_t i = 0; i < first.size(); i++) {
            if (first[i] != 0) {
                ASSERT_GT(abs(first[i]), abs(second[i]));

            } else {
                ASSERT_EQ(first[i], second[i]);
            }
        }
    }

    void assertSequentialGains(std::vector<int> gainValues, bool isIncreasing) {
        std::vector<float> baseOutput(kBufferSize);

        // Process a reference output buffer with 0 gain which gives compressed input values
        ASSERT_EQ(SetParameters(kZeroGain), EX_NONE);
        ASSERT_NO_FATAL_FAILURE(process());
        baseOutput = mOutputBuffer;

        // Compare the outputs for increasing gain
        for (int gain : gainValues) {
            // Setting the parameters
            if (SetParameters(gain) != EX_NONE) {
                GTEST_SKIP() << "Gains not supported.";
            }

            // Add gains to the input buffer
            ASSERT_NO_FATAL_FAILURE(process());

            // Compare the mOutputBuffer values with baseOutput and update it
            if (isIncreasing) {
                ASSERT_NO_FATAL_FAILURE(assertGreaterGain(mOutputBuffer, baseOutput));
            } else {
                ASSERT_NO_FATAL_FAILURE(assertGreaterGain(baseOutput, mOutputBuffer));
            }

            baseOutput = mOutputBuffer;
        }
    }

    std::unique_ptr<StatusMQ> mStatusMQ;
    std::unique_ptr<DataMQ> mInputMQ;
    std::unique_ptr<DataMQ> mOutputMQ;

    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    static constexpr float kBufferSize = 128;
};

TEST_P(LoudnessEnhancerDataTest, IncreasingGains) {
    static const std::vector<int> kIncreasingGains = {50, 100};

    assertSequentialGains(kIncreasingGains, true /*isIncreasing*/);
}

TEST_P(LoudnessEnhancerDataTest, DecreasingGains) {
    static const std::vector<int> kDecreasingGains = {-50, -100};

    assertSequentialGains(kDecreasingGains, false /*isIncreasing*/);
}

TEST_P(LoudnessEnhancerDataTest, MinimumGain) {
    // Setting the parameters
    if (SetParameters(kMinGain) != EX_NONE) {
        GTEST_SKIP() << "Minimum integer value not supported";
    }

    // Add gains to the input buffer
    ASSERT_NO_FATAL_FAILURE(process());

    // Validate that mOutputBuffer has 0 values for INT_MIN gain
    for (size_t i = 0; i < mOutputBuffer.size(); i++) {
        ASSERT_FLOAT_EQ(mOutputBuffer[i], 0);
    }
}

TEST_P(LoudnessEnhancerDataTest, MaximumGain) {
    // Setting the parameters
    if (SetParameters(kMaxGain) != EX_NONE) {
        GTEST_SKIP() << "Maximum integer value not supported";
    }

    // Add gains to the input buffer
    ASSERT_NO_FATAL_FAILURE(process());

    // Validate that mOutputBuffer reaches to kMaxAudioSample for INT_MAX gain
    for (size_t i = 0; i < mOutputBuffer.size(); i++) {
        if (mInputBuffer[i] != 0) {
            EXPECT_NEAR(kMaxAudioSample, abs(mOutputBuffer[i]), kAbsError);
        } else {
            ASSERT_EQ(mOutputBuffer[i], mInputBuffer[i]);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancerTest, LoudnessEnhancerParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidLoudnessEnhancer())),
                           testing::ValuesIn(kGainMbValues)),
        [](const testing::TestParamInfo<LoudnessEnhancerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string gainMb = std::to_string(std::get<PARAM_GAIN_MB>(info.param));
            std::string name = getPrefix(descriptor) + "_gainMb_" + gainMb;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancerTest, LoudnessEnhancerDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidLoudnessEnhancer()))),
        [](const testing::TestParamInfo<LoudnessEnhancerDataTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name = getPrefix(descriptor);
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerParamTest);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
