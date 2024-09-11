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

#include <unordered_set>

#define LOG_TAG "VtsHalVisualizerTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVisualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Visualizer;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_CAPTURE_SIZE,
    PARAM_SCALING_MODE,
    PARAM_MEASUREMENT_MODE,
    PARAM_LATENCY,
};
using VisualizerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int, Visualizer::ScalingMode,
                   Visualizer::MeasurementMode, int>;

class VisualizerParamTest : public ::testing::TestWithParam<VisualizerParamTestParam>,
                            public EffectHelper {
  public:
    VisualizerParamTest()
        : mCaptureSize(std::get<PARAM_CAPTURE_SIZE>(GetParam())),
          mScalingMode(std::get<PARAM_SCALING_MODE>(GetParam())),
          mMeasurementMode(std::get<PARAM_MEASUREMENT_MODE>(GetParam())),
          mLatency(std::get<PARAM_LATENCY>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());

        size_t channelCount =
                getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                        AudioChannelLayout::LAYOUT_STEREO));
        mBufferSizeInFrames = kInputFrameCount * channelCount;
        mInputBuffer.resize(mBufferSizeInFrames);
        generateInputBuffer(mInputBuffer, 0, true, channelCount, kMaxAudioSampleValue);

        mOutputBuffer.resize(mBufferSizeInFrames);
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
        mVersion = EffectFactoryHelper::getHalVersion(mFactory);

        // Creating AidlMessageQueues
        mStatusMQ = std::make_unique<EffectHelper::StatusMQ>(mOpenEffectReturn.statusMQ);
        mInputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn.inputDataMQ);
        mOutputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn.outputDataMQ);

        // Check AidlMessageQueues are not null
        ASSERT_TRUE(mStatusMQ->isValid());
        ASSERT_TRUE(mInputMQ->isValid());
        ASSERT_TRUE(mOutputMQ->isValid());
    }

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    // Add gains to the mInputBuffer and store processed output to mOutputBuffer
    void processAndWriteToOutput() {
        // Enabling the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
        ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

        // Write from buffer to message queues and calling process
        EXPECT_NO_FATAL_FAILURE(
                EffectHelper::writeToFmq(mStatusMQ, mInputMQ, mInputBuffer, mVersion));

        // Read the updated message queues into buffer
        EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(mStatusMQ, 1, mOutputMQ,
                                                          mOutputBuffer.size(), mOutputBuffer));

        // Disable the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mCaptureSize;
    Visualizer::ScalingMode mScalingMode = Visualizer::ScalingMode::NORMALIZED;
    Visualizer::MeasurementMode mMeasurementMode = Visualizer::MeasurementMode::NONE;
    int mLatency = 0;
    int mVersion = 0;
    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    size_t mBufferSizeInFrames;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    std::unique_ptr<StatusMQ> mStatusMQ;
    std::unique_ptr<DataMQ> mInputMQ;
    std::unique_ptr<DataMQ> mOutputMQ;
    bool mAllParamsValid = true;

    void SetAndGetParameters() {
        for (auto& it : mCommonTags) {
            auto& tag = it.first;
            auto& vs = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isParameterValid<Visualizer, Range::visualizer>(vs, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;
            if (expected == EX_ILLEGAL_ARGUMENT) mAllParamsValid = false;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::visualizer>(vs);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                Visualizer::Id vsId;
                vsId.set<Visualizer::Id::commonTag>(tag);
                id.set<Parameter::Id::visualizerTag>(vsId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam))
                        << " with: " << id.toString();
                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addCaptureSizeParam(int captureSize) {
        mCommonTags.push_back({Visualizer::captureSamples,
                               Visualizer::make<Visualizer::captureSamples>(captureSize)});
    }

    void addScalingModeParam(Visualizer::ScalingMode scalingMode) {
        mCommonTags.push_back(
                {Visualizer::scalingMode, Visualizer::make<Visualizer::scalingMode>(scalingMode)});
    }

    void addMeasurementModeParam(Visualizer::MeasurementMode measurementMode) {
        mCommonTags.push_back({Visualizer::measurementMode,
                               Visualizer::make<Visualizer::measurementMode>(measurementMode)});
    }

    void addLatencyParam(int latency) {
        mCommonTags.push_back(
                {Visualizer::latencyMs, Visualizer::make<Visualizer::latencyMs>(latency)});
    }

    static std::unordered_set<Visualizer::MeasurementMode> getMeasurementModeValues() {
        return {ndk::enum_range<Visualizer::MeasurementMode>().begin(),
                ndk::enum_range<Visualizer::MeasurementMode>().end()};
    }

    static std::unordered_set<Visualizer::ScalingMode> getScalingModeValues() {
        return {ndk::enum_range<Visualizer::ScalingMode>().begin(),
                ndk::enum_range<Visualizer::ScalingMode>().end()};
    }

  private:
    std::vector<std::pair<Visualizer::Tag, Visualizer>> mCommonTags;
    void CleanUp() { mCommonTags.clear(); }
};

TEST_P(VisualizerParamTest, SetAndGetCaptureSize) {
    EXPECT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
    SetAndGetParameters();
}

TEST_P(VisualizerParamTest, SetAndGetScalingMode) {
    EXPECT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
    SetAndGetParameters();
}

TEST_P(VisualizerParamTest, SetAndGetMeasurementMode) {
    EXPECT_NO_FATAL_FAILURE(addMeasurementModeParam(mMeasurementMode));
    SetAndGetParameters();
}

TEST_P(VisualizerParamTest, SetAndGetLatency) {
    EXPECT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
    SetAndGetParameters();
}

TEST_P(VisualizerParamTest, testCaptureSampleBufferSizeAndOutput) {
    EXPECT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
    EXPECT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
    EXPECT_NO_FATAL_FAILURE(addMeasurementModeParam(mMeasurementMode));
    EXPECT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
    SetAndGetParameters();
    if (!mAllParamsValid) return;

    Parameter getParam;
    Parameter::Id id;
    Visualizer::Id vsId;
    vsId.set<Visualizer::Id::commonTag>(Visualizer::captureSampleBuffer);
    id.set<Parameter::Id::visualizerTag>(vsId);
    EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam)) << " with: " << id.toString();

    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput());

    std::vector<uint8_t> captureBuffer = getParam.get<Parameter::specific>()
                                                 .get<Parameter::Specific::visualizer>()
                                                 .get<Visualizer::captureSampleBuffer>();

    ASSERT_EQ((size_t)mCaptureSize, captureBuffer.size());
    ASSERT_EQ(mInputBuffer, mOutputBuffer);
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        VisualizerParamTest, VisualizerParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidVisualizer())),
                testing::ValuesIn(EffectHelper::getTestValueSet<Visualizer, int, Range::visualizer,
                                                                Visualizer::captureSamples>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(VisualizerParamTest::getScalingModeValues()),
                testing::ValuesIn(VisualizerParamTest::getMeasurementModeValues()),
                testing::ValuesIn(EffectHelper::getTestValueSet<Visualizer, int, Range::visualizer,
                                                                Visualizer::latencyMs>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<VisualizerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string captureSize = std::to_string(std::get<PARAM_CAPTURE_SIZE>(info.param));
            std::string scalingMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_SCALING_MODE>(info.param));
            std::string measurementMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_MEASUREMENT_MODE>(info.param));
            std::string latency = std::to_string(std::get<PARAM_LATENCY>(info.param));

            std::string name = getPrefix(descriptor) + "_captureSize" + captureSize +
                               "_scalingMode" + scalingMode + "_measurementMode" + measurementMode +
                               "_latency" + latency;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VisualizerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
