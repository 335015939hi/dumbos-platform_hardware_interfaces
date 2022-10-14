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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define LOG_TAG "VtsHalAudioEffectTest"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <fmq/AidlMessageQueue.h>

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "TestUtils.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;

class AudioEffectTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.ConnectToFactoryService());
        CreateEffects();
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
    }

    void OpenEffects() {
        auto open = [&](const std::shared_ptr<IEffect>& effect) {
            IEffect::OpenEffectReturn ret;
            EXPECT_IS_OK(effect->open(mCommon, mSpecific, &ret));
            EffectParam params;
            params.statusMQ = std::make_unique<StatusMQ>(ret.statusMQ);
            params.inputMQ = std::make_unique<DataMQ>(ret.inputDataMQ);
            params.outputMQ = std::make_unique<DataMQ>(ret.outputDataMQ);
            mEffectParams.push_back(std::move(params));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(open));
    }

    void CloseEffects(const binder_status_t status = EX_NONE) {
        auto close = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_STATUS(status, effect->close());
        };

        EXPECT_NO_FATAL_FAILURE(ForEachEffect(close));
    }

    void CreateEffects(const int n = 1) {
        for (int i = 0; i < n; i++) {
            ASSERT_NO_FATAL_FAILURE(mFactoryHelper.QueryAndCreateAllEffects());
        }
    }

    void DestroyEffects(const binder_status_t status = EX_NONE, const int remaining = 0) {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.DestroyEffects(status, remaining));
    }

    void GetEffectDescriptors() {
        auto get = [](const std::shared_ptr<IEffect>& effect) {
            Descriptor desc;
            EXPECT_IS_OK(effect->getDescriptor(&desc));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(get));
    }

    void CommandEffects(CommandId command) {
        auto close = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_IS_OK(effect->command(command));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(close));
    }

    void CommandEffectsExpectStatus(CommandId command, const binder_status_t status) {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_STATUS(status, effect->command(command));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    void ExpectState(State expected) {
        auto get = [&](const std::shared_ptr<IEffect>& effect) {
            State state = State::INIT;
            EXPECT_IS_OK(effect->getState(&state));
            EXPECT_EQ(expected, state);
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(get));
    }

    void SetParameter() {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            Parameter param;
            param.set<Parameter::common>(mCommon);
            EXPECT_IS_OK(effect->setParameter(param));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    void VerifyParameters() {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            Parameter paramCommonGet = Parameter(), paramCommonExpect = Parameter();
            Parameter::Id id;
            id.set<Parameter::Id::commonTag>(0);
            paramCommonExpect.set<Parameter::common>(mCommon);
            EXPECT_IS_OK(effect->getParameter(id, &paramCommonGet));
            EXPECT_EQ(paramCommonExpect, paramCommonGet)
                    << paramCommonExpect.toString() << " vs " << paramCommonGet.toString();
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    template <typename Functor>
    void ForEachEffect(Functor functor) {
        auto effectMap = mFactoryHelper.GetEffectMap();
        for (const auto& it : effectMap) {
            SCOPED_TRACE(it.second.toString());
            functor(it.first);
        }
    }

    enum class IO : char { INPUT = 0, OUTPUT = 1, INOUT = 2 };

    void initParamCommonFormat(IO io = IO::INOUT,
                               const AudioFormatDescription& format = mDefaultFormat) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.base.format = format;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.base.format = format;
        }
    }

    void initParamCommonSampleRate(IO io = IO::INOUT, const int& sampleRate = 48000) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.base.sampleRate = sampleRate;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.base.sampleRate = sampleRate;
        }
    }

    void initParamCommonFrameCount(IO io = IO::INOUT, const long& frameCount = 48000) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.frameCount = frameCount;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.frameCount = frameCount;
        }
    }
    void initParamCommon(int session = -1, int ioHandle = -1,
                         AudioDeviceType deviceType = AudioDeviceType::NONE,
                         int iSampleRate = 48000, int oSampleRate = 48000, long iFrameCount = 0x100,
                         long oFrameCount = 0x100) {
        mCommon.session = session;
        mCommon.ioHandle = ioHandle;
        mCommon.device.type = deviceType;

        auto& input = mCommon.input;
        auto& output = mCommon.output;
        input.base.sampleRate = iSampleRate;
        input.base.channelMask = mInputChannelLayout;
        input.frameCount = iFrameCount;
        output.base.sampleRate = oSampleRate;
        output.base.channelMask = mOutputChannelLayout;
        output.frameCount = oFrameCount;
        inputFrameSize = android::hardware::audio::common::getFrameSizeInBytes(
                input.base.format, input.base.channelMask);
        outputFrameSize = android::hardware::audio::common::getFrameSizeInBytes(
                output.base.format, output.base.channelMask);
    }

    void initParamSpecific(Parameter::Specific::Tag tag = Parameter::Specific::equalizer) {
        switch (tag) {
            case Parameter::Specific::equalizer:
                mSpecific.set<Parameter::Specific::equalizer>();
                break;
            default:
                return;
        }
    }

    // usually this function only call once.
    void PrepareInputData(size_t s = mWriteMQSize) {
        size_t maxInputSize = s;
        for (auto& it : mEffectParams) {
            auto& mq = it.inputMQ;
            EXPECT_NE(nullptr, mq);
            EXPECT_TRUE(mq->isValid());
            const size_t bytesToWrite = mq->availableToWrite();
            EXPECT_EQ(inputFrameSize * mCommon.input.frameCount, bytesToWrite);
            EXPECT_NE(0UL, bytesToWrite);
            EXPECT_TRUE(s <= bytesToWrite);
            maxInputSize = std::max(maxInputSize, bytesToWrite);
        }
        mInputBuffer.resize(maxInputSize);
        std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0x5a);
    }

    void writeToFmq(size_t s = mWriteMQSize) {
        for (auto& it : mEffectParams) {
            auto& mq = it.inputMQ;
            EXPECT_NE(nullptr, mq);
            const size_t bytesToWrite = mq->availableToWrite();
            EXPECT_NE(0Ul, bytesToWrite);
            EXPECT_TRUE(s <= bytesToWrite);
            EXPECT_TRUE(mq->write(mInputBuffer.data(), s));
        }
    }

    void readFromFmq(size_t expectSize = mWriteMQSize) {
        for (auto& it : mEffectParams) {
            IEffect::Status status{};
            auto& statusMq = it.statusMQ;
            EXPECT_NE(nullptr, statusMq);
            EXPECT_TRUE(statusMq->readBlocking(&status, 1));
            EXPECT_EQ(STATUS_OK, status.status);
            EXPECT_EQ(expectSize, (unsigned)status.fmqByteProduced);

            auto& outputMq = it.outputMQ;
            EXPECT_NE(nullptr, outputMq);
            EXPECT_EQ(expectSize, outputMq->availableToRead());
        }
    }

    void setInputChannelLayout(AudioChannelLayout input) { mInputChannelLayout = input; }
    void setOutputChannelLayout(AudioChannelLayout output) { mOutputChannelLayout = output; }

    EffectFactoryHelper mFactoryHelper = EffectFactoryHelper(GetParam());

    static const AudioFormatDescription mDefaultFormat;
    static const size_t mWriteMQSize = 0x400;

  private:
    AudioChannelLayout mInputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);
    AudioChannelLayout mOutputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);

    Parameter::Common mCommon;
    Parameter::Specific mSpecific;

    size_t inputFrameSize, outputFrameSize;
    std::vector<int8_t> mInputBuffer;  // reuse same buffer for all effects testing

    typedef ::android::AidlMessageQueue<
            IEffect::Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    class EffectParam {
      public:
        std::unique_ptr<StatusMQ> statusMQ;
        std::unique_ptr<DataMQ> inputMQ;
        std::unique_ptr<DataMQ> outputMQ;
    };
    std::vector<EffectParam> mEffectParams;
};

const AudioFormatDescription AudioEffectTest::mDefaultFormat = {
        .type = AudioFormatType::PCM, .pcm = PcmType::INT_16_BIT, .encoding = ""};

TEST_P(AudioEffectTest, OpenEffectTest) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
}

TEST_P(AudioEffectTest, OpenAndCloseEffect) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffectTest, CloseUnopenedEffectTest) {
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffectTest, DoubleOpenCloseEffects) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffectTest, GetDescriptors) {
    EXPECT_NO_FATAL_FAILURE(GetEffectDescriptors());
}

TEST_P(AudioEffectTest, DescriptorIdExistAndUnique) {
    auto checker = [&](const std::shared_ptr<IEffect>& effect) {
        Descriptor desc;
        std::vector<Descriptor::Identity> idList;
        EXPECT_IS_OK(effect->getDescriptor(&desc));
        mFactoryHelper.QueryEffects(desc.common.id.type, desc.common.id.uuid, &idList);
        EXPECT_EQ(idList.size(), 1UL);
    };
    EXPECT_NO_FATAL_FAILURE(ForEachEffect(checker));

    // Check unique with a set
    auto stringHash = [](const Descriptor::Identity& id) {
        return std::hash<std::string>()(id.toString());
    };
    auto vec = mFactoryHelper.GetCompleteEffectIdList();
    std::unordered_set<Descriptor::Identity, decltype(stringHash)> idSet(0, stringHash);
    for (auto it : vec) {
        EXPECT_EQ(idSet.count(it), 0UL);
        idSet.insert(it);
    }
}

/// State testing.
// An effect instance is in INIT state by default after it was created.
TEST_P(AudioEffectTest, InitStateAfterCreation) {
    ExpectState(State::INIT);
}

// An effect instance transfer to INIT state after it was open successfully with IEffect.open().
TEST_P(AudioEffectTest, IdleStateAfterOpen) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance is in PROCESSING state after it receive an START command.
TEST_P(AudioEffectTest, ProcessingStateAfterStart) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to IDLE state after Command.Id.STOP in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterStop) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to IDLE state after Command.Id.RESET in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterReset) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to INIT if instance receive a close() call.
TEST_P(AudioEffectTest, InitStateAfterClose) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    ExpectState(State::INIT);
}

// An effect instance shouldn't accept any command before open.
TEST_P(AudioEffectTest, NoCommandAcceptedBeforeOpen) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::START, EX_ILLEGAL_STATE));
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::STOP, EX_ILLEGAL_STATE));
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::RESET, EX_ILLEGAL_STATE));
    ExpectState(State::INIT);
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, StopCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, ResetCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and STOP command.
TEST_P(AudioEffectTest, RepeatStartAndStop) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and RESET command.
TEST_P(AudioEffectTest, RepeatStartAndReset) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and STOP command, try to close at PROCESSING state.
TEST_P(AudioEffectTest, CloseProcessingStateEffects) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CloseEffects(EX_ILLEGAL_STATE));
    // cleanup
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyOpenEffects) {
    // cleanup all effects.
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    ASSERT_NO_FATAL_FAILURE(DestroyEffects());

    // open effects, destroy without close, expect to get EX_ILLEGAL_STATE status.
    EXPECT_NO_FATAL_FAILURE(CreateEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(DestroyEffects(EX_ILLEGAL_STATE, 1));
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

/// Parameter testing.
// Verify parameters pass in open can be successfully get.
TEST_P(AudioEffectTest, VerifyParametersAfterOpen) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetParameter) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetParameterInProcessing) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Parameters kept after reset.
TEST_P(AudioEffectTest, ResetAndVerifyParameter) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Multiple instances of same implementation running.
TEST_P(AudioEffectTest, MultipleInstancesRunning) {
    EXPECT_NO_FATAL_FAILURE(CreateEffects(3));
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Send data to effects and expect it to consume by check statusMQ.
TEST_P(AudioEffectTest, ExpectEffectsToConsumeDataInMQ) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    PrepareInputData(mWriteMQSize);

    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    writeToFmq(mWriteMQSize);
    readFromFmq(mWriteMQSize);

    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    // cleanup
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

INSTANTIATE_TEST_SUITE_P(AudioEffectTestTest, AudioEffectTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
