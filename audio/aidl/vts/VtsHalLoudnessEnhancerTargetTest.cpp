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
enum ParamName { PARAM_INSTANCE_NAME, PARAM_GAIN_MB };
using LoudnessEnhancerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>;

// Every int 32 bit value is a valid gain, so testing the corner cases and one regular value.
// TODO : Update the test values once range/capability is updated by implementation.
const std::vector<int> kGainMbValues = {0, 50, 100};

class LoudnessEnhancerParamTest : public ::testing::TestWithParam<LoudnessEnhancerParamTestParam>,
                                  public EffectHelper {
  public:
    LoudnessEnhancerParamTest() : mParamGainMb(std::get<PARAM_GAIN_MB>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));
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
    IEffect::OpenEffectReturn ret;  // Added here because its requried to create AidlMessageQueues

    Descriptor mDescriptor;
    int mParamGainMb = 0;
    static std::vector<float> baseOutput;  // Output buffer for comparision

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

  private:
    std::vector<std::pair<LoudnessEnhancer::Tag, LoudnessEnhancer>> mTags;
    void CleanUp() { mTags.clear(); }
};

std::vector<float> LoudnessEnhancerParamTest::baseOutput(128, 0);

TEST_P(LoudnessEnhancerParamTest, SetAndGetGainMb) {
    EXPECT_NO_FATAL_FAILURE(addGainMbParam(mParamGainMb));
    SetAndGetParameters();

    // Creating AidlMessageQueues
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    // Enabling the process
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    // Randomized input buffer, values between 0 to 255
    std::vector<float> buffer = {
            195.735, 145.43,  161.136, 42.9741,  131.744, 234.221, 80.4335, 175.735,  155.454,
            127.618, 242.554, 163.779, 67.8213,  62.5858, 77.0776, 64.3263, 222.162,  159.854,
            105.081, 105.251, 28.6376, 154.089,  80.2321, 171.056, 103.452, 0.953247, 35.9135,
            169.298, 211.8,   199.948, 248.727,  129.377, 160.523, 175.766, 211.807,  88.9537,
            228.705, 54.7023, 26.295,  227.601,  27.0066, 6.5042,  218.07,  180.24,   236.703,
            128.861, 91.2668, 114.194, 140.262,  236.419, 148.583, 39.4713, 8.81902,  213.231,
            190.163, 9.96331, 74.2867, 0.137829, 165.987, 75.7569, 138.177, 245.994,  125.189,
            185.149, 146.433, 78.9154, 123.374,  109.537, 43.4098, 8.01371, 168.579,  254.681,
            252.643, 241.85,  249.948, 68.7457,  57.1038, 22.8576, 86.3633, 148.062,  163.746,
            179.18,  230.935, 243.094, 227.783,  72.6633, 204.334, 247.363, 187.917,  48.2008,
            204.771, 171.84,  119.595, 66.2766,  100.758, 120.146, 16.9248, 25.9979,  50.407,
            237.631, 233.644, 111.369, 67.586,   182.657, 170.955, 156.903, 229.487,  8.56596,
            9.07093, 76.963,  159.366, 227.008,  65.2445, 165.588, 122.605, 109.778,  139.705,
            215.546, 240.516, 75.7411, 80.1789,  72.377,  59.9478, 220.775, 244.744,  9.13637,
            93.09,   41.0474};

    // Write from buffer to messegequeues and calling process
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));

    // Read the updated messegequeues into bufffer
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    // Compare the increase in the buffer values with baseOutput buffer and update baseOutput buffer
    for (size_t i = 0; i < buffer.size(); i++) {
        ASSERT_GE(buffer[i], baseOutput[i]);
        baseOutput[i] = buffer[i];
    }

    // Disble the process
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));

    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));
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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}