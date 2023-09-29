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

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME };
using LoudnessEnhancerParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>>;

// Every int 32 bit value is a valid gain, so testing the corner cases and one regular value.
// TODO : Update the test values once range/capability is updated by implementation.

class LoudnessEnhancerParamTest : public ::testing::TestWithParam<LoudnessEnhancerParamTestParam>,
                                  public EffectHelper {
  public:
    LoudnessEnhancerParamTest() {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));

        // Creating AidlMessageQueues
        statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
        inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
        outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

        ASSERT_NE(nullptr, mEffect);
    }
    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        LoudnessEnhancer le = LoudnessEnhancer::make<LoudnessEnhancer::gainMb>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::loudnessEnhancer>(le);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;

    std::unique_ptr<AidlMessageQueue<IEffect::Status,
                                     ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>>
            statusMQ;
    std::unique_ptr<android::AidlMessageQueue<
            float, aidl::android::hardware::common::fmq::SynchronizedReadWrite>>
            inputMQ;
    std::unique_ptr<android::AidlMessageQueue<
            float, aidl::android::hardware::common::fmq::SynchronizedReadWrite>>
            outputMQ;

    Descriptor mDescriptor;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& le = it.second;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::loudnessEnhancer>(le);
            expectParam.set<Parameter::specific>(specific);
            // All values are valid, set parameter should succeed
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

            // get parameter
            Parameter getParam;
            Parameter::Id id;
            LoudnessEnhancer::Id leId;
            leId.set<LoudnessEnhancer::Id::commonTag>(tag);
            id.set<Parameter::Id::loudnessEnhancerTag>(leId);
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

            EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                             << "\ngetParam:" << getParam.toString();
        }
    }

    void addGainMbParam(int gainMb) {
        LoudnessEnhancer le;
        le.set<LoudnessEnhancer::gainMb>(gainMb);
        mTags.push_back({LoudnessEnhancer::gainMb, le});
    }

    void generateInputBuffer(std::vector<float>& buffer) {
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = static_cast<float>(std::rand()) / RAND_MAX * 255;
        }
    }

    void process(std::vector<float>& input, std::vector<float>& output, int gain) {
        // Setting the parameters
        EXPECT_NO_FATAL_FAILURE(addGainMbParam(gain));
        SetAndGetParameters();

        // Check if AidlMessageQueues are not null
        ASSERT_TRUE(statusMQ->isValid());
        ASSERT_TRUE(inputMQ->isValid());
        ASSERT_TRUE(outputMQ->isValid());

        // Enabling the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
        ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

        // Write from buffer to message queues and calling process
        EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, input));

        // Read the updated message queues into buffer
        EXPECT_NO_FATAL_FAILURE(
                EffectHelper::readFromFmq(statusMQ, 1, outputMQ, output.size(), output));

        // Disable the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    }

  private:
    std::vector<std::pair<LoudnessEnhancer::Tag, LoudnessEnhancer>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(LoudnessEnhancerParamTest, SetAndGetGainMb) {
    int gainValues[] = {INT_MIN, 100, INT_MAX};

    for (int gain : gainValues) {
        EXPECT_NO_FATAL_FAILURE(addGainMbParam(gain));
        SetAndGetParameters();
    }
}

TEST_P(LoudnessEnhancerParamTest, LoudnessValidation) {
    std::vector<float> inputBuffer(128);
    // Fill inputBuffer with random values between 0 to 255
    generateInputBuffer(inputBuffer);

    std::vector<float> baseOutput(128);

    // Create a baseOutput with 0 gain which only has compressed values
    // It is a reference to compare with the outputs of different gains
    process(inputBuffer, baseOutput, 0);

    // Compare the outputs for increasing gain
    int gainValues[] = {50, 100, INT_MAX};

    for (int gain : gainValues) {
        std::vector<float> outputBuffer(128);

        // Add gains to the inputBuffer
        process(inputBuffer, outputBuffer, gain);

        // Compare the increase in the outputBuffer values with baseOutput and update it
        for (size_t i = 0; i < inputBuffer.size(); i++) {
            ASSERT_GE(outputBuffer[i], baseOutput[i]);
            baseOutput[i] = outputBuffer[i];
        }
    }
}

TEST_P(LoudnessEnhancerParamTest, MinimumGain) {
    std::vector<float> inputBuffer(128);
    // Fill input buffer with random values between 0 to 255
    generateInputBuffer(inputBuffer);

    std::vector<float> outputBuffer(128);

    // Add gains to the input buffer
    process(inputBuffer, outputBuffer, INT_MIN);

    // Validate outputBuffer has 0 values for INT_MIN gain
    for (size_t i = 0; i < outputBuffer.size(); i++) {
        ASSERT_EQ(outputBuffer[i], 0);
    }
}

INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancerTest, LoudnessEnhancerParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidLoudnessEnhancer()))),
        [](const testing::TestParamInfo<LoudnessEnhancerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name = getPrefix(descriptor);
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
