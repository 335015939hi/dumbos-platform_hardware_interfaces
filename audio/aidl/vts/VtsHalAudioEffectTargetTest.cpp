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

#define LOG_TAG "VtsHalAudioEffectTargetTest"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <Utils.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <fmq/AidlMessageQueue.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;

enum ParamName { PARAM_INSTANCE_NAME };
using EffectTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>>;

class AudioEffectTest : public testing::TestWithParam<EffectTestParam>, public EffectHelper {
  public:
    AudioEffectTest() { std::tie(mFactory, mIdentity) = std::get<PARAM_INSTANCE_NAME>(GetParam()); }

    void SetUp() override {}
    void TearDown() override {}

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    Descriptor::Identity mIdentity;
};

TEST_P(AudioEffectTest, SetupAndTearDown) {
    // Intentionally empty test body.
}

TEST_P(AudioEffectTest, CreateAndDestroy) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    destroy(mFactory, effect);
}

TEST_P(AudioEffectTest, OpenAndClose) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    close(effect);
    destroy(mFactory, effect);
}

TEST_P(AudioEffectTest, CloseUnopenedEffect) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    close(effect);
    destroy(mFactory, effect);
}

TEST_P(AudioEffectTest, DoubleOpenAndClose) {
    std::shared_ptr<IEffect> effect1, effect2;
    create(mFactory, effect1, mIdentity);
    create(mFactory, effect2, mIdentity);
    open(effect1);
    open(effect2, 1 /* session */);
    close(effect1);
    close(effect2);
    destroy(mFactory, effect1);
    destroy(mFactory, effect2);
}

TEST_P(AudioEffectTest, TripleOpenAndClose) {
    std::shared_ptr<IEffect> effect1, effect2, effect3;
    create(mFactory, effect1, mIdentity);
    create(mFactory, effect2, mIdentity);
    create(mFactory, effect3, mIdentity);
    open(effect1);
    open(effect2, 1 /* session */);
    open(effect3, 2 /* session */);
    close(effect1);
    close(effect2);
    close(effect3);
    destroy(mFactory, effect1);
    destroy(mFactory, effect2);
    destroy(mFactory, effect3);
}

TEST_P(AudioEffectTest, GetDescritorBeforeOpen) {
    std::shared_ptr<IEffect> effect;
    Descriptor desc;
    create(mFactory, effect, mIdentity);
    getDescriptor(effect, desc);
    EXPECT_EQ(mIdentity.toString(), desc.common.id.toString());
    EXPECT_NE("", desc.common.name);
    EXPECT_NE("", desc.common.implementor);
    destroy(mFactory, effect);
}

TEST_P(AudioEffectTest, GetDescritorAfterOpen) {
    std::shared_ptr<IEffect> effect;
    Descriptor beforeOpen, afterOpen, afterClose;
    create(mFactory, effect, mIdentity);
    getDescriptor(effect, beforeOpen);
    open(effect);
    getDescriptor(effect, afterOpen);
    EXPECT_EQ(beforeOpen.toString(), afterOpen.toString()) << "\n"
                                                           << beforeOpen.toString() << "\n"
                                                           << afterOpen.toString();
    close(effect);
    getDescriptor(effect, afterClose);
    EXPECT_EQ(beforeOpen.toString(), afterClose.toString()) << "\n"
                                                            << beforeOpen.toString() << "\n"
                                                            << afterClose.toString();
    destroy(mFactory, effect);
}

TEST_P(AudioEffectTest, DescriptorExistAndUnique) {
    std::shared_ptr<IEffect> effect;
    Descriptor desc;

    auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor);
    std::set<Descriptor::Identity> idSet;
    for (const auto& it : descList) {
        auto& id = it.second;
        EXPECT_EQ(0ul, idSet.count(id));
        idSet.insert(id);
    }

    create(mFactory, effect, mIdentity);
    getDescriptor(effect, desc);
    EXPECT_EQ(1ul, idSet.count(desc.common.id));
    destroy(mFactory, effect);
}

/// State testing.
// An effect instance is in INIT state by default after it was created.
TEST_P(AudioEffectTest, InitStateAfterCreation) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    expectState(effect, State::INIT);
    destroy(mFactory, effect);
}

// An effect instance transfer to IDLE state after IEffect.open().
TEST_P(AudioEffectTest, IdleStateAfterOpen) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// An effect instance is in PROCESSING state after it receive an START command.
TEST_P(AudioEffectTest, ProcessingStateAfterStart) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    expectState(effect, State::INIT);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    close(effect);
    destroy(mFactory, effect);
}

// An effect instance transfer to IDLE state after Command.Id.STOP in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterStop) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// An effect instance transfer to IDLE state after Command.Id.RESET in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterReset) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// An effect instance transfer to INIT after IEffect.close().
TEST_P(AudioEffectTest, InitStateAfterClose) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    command(effect, CommandId::STOP);
    close(effect);
    expectState(effect, State::INIT);
    destroy(mFactory, effect);
}

// An effect instance shouldn't accept any command before open.
TEST_P(AudioEffectTest, NoCommandAcceptedBeforeOpen) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    command(effect, CommandId::START, EX_ILLEGAL_STATE);
    command(effect, CommandId::STOP, EX_ILLEGAL_STATE);
    command(effect, CommandId::RESET, EX_ILLEGAL_STATE);
    open(effect);
    close(effect);
    destroy(mFactory, effect);
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, StopCommandInIdleStateNoOp) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// No-op when receive RESET command in IDLE state.
TEST_P(AudioEffectTest, ResetCommandInIdleStateNoOp) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// Repeat START and STOP command.
TEST_P(AudioEffectTest, RepeatStartAndStop) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// Repeat START and RESET command.
TEST_P(AudioEffectTest, RepeatStartAndReset) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// Try to close an effect instance at PROCESSING state.
TEST_P(AudioEffectTest, CloseProcessingStateEffects) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    close(effect, EX_ILLEGAL_STATE);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyOpenEffects) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);

    destroy(mFactory, effect, EX_ILLEGAL_STATE);
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyProcessingEffects) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    destroy(mFactory, effect, EX_ILLEGAL_STATE);
}

TEST_P(AudioEffectTest, NormalSequenceStates) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    expectState(effect, State::INIT);
    open(effect);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);
    close(effect);
    destroy(mFactory, effect);
}

/// Parameter testing.
// Verify parameters pass in open can be successfully get.
TEST_P(AudioEffectTest, VerifyCommonParametersAfterOpen) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon();
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);

    Parameter get = Parameter(), expect = Parameter();
    expect.set<Parameter::common>(common);
    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(expect, get) << expect.toString() << " vs " << get.toString();

    close(effect);
    destroy(mFactory, effect);
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetCommonParameter) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter get = Parameter(), set = Parameter();
    set.set<Parameter::common>(common);
    EXPECT_IS_OK(effect->setParameter(set));

    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(set, get) << set.toString() << " vs " << get.toString();

    close(effect);
    destroy(mFactory, effect);
}

// Verify parameters set and get in PROCESSING state.
TEST_P(AudioEffectTest, SetAndGetParameterInProcessing) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter get = Parameter(), set = Parameter();
    set.set<Parameter::common>(common);
    EXPECT_IS_OK(effect->setParameter(set));

    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(set, get) << set.toString() << " vs " << get.toString();

    command(effect, CommandId::STOP);
    close(effect);
    destroy(mFactory, effect);
}

// Verify parameters set and get in IDLE state.
TEST_P(AudioEffectTest, SetAndGetParameterInIdle) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter get = Parameter(), set = Parameter();
    set.set<Parameter::common>(common);
    EXPECT_IS_OK(effect->setParameter(set));

    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(set, get) << set.toString() << " vs " << get.toString();

    close(effect);
    destroy(mFactory, effect);
}

// Verify Parameters kept after stop.
TEST_P(AudioEffectTest, SetAndGetParameterAfterStop) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter get = Parameter(), set = Parameter();
    set.set<Parameter::common>(common);
    EXPECT_IS_OK(effect->setParameter(set));

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(set, get) << set.toString() << " vs " << get.toString();

    close(effect);
    destroy(mFactory, effect);
}

// Verify Parameters kept after reset.
TEST_P(AudioEffectTest, SetAndGetParameterAfterReset) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);
    open(effect);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter get = Parameter(), set = Parameter();
    set.set<Parameter::common>(common);
    EXPECT_IS_OK(effect->setParameter(set));

    command(effect, CommandId::RESET);
    expectState(effect, State::IDLE);

    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(effect->getParameter(id, &get));
    EXPECT_EQ(set, get) << set.toString() << " vs " << get.toString();

    close(effect);
    destroy(mFactory, effect);
}

/// Data processing test
// Send data to effects and expect it to be consumed by checking statusMQ.
TEST_P(AudioEffectTest, ConsumeDataInProcessingState) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    close(effect);
    destroy(mFactory, effect);
}

// Send data to effects and expect it to be consumed after effect restart.
TEST_P(AudioEffectTest, ConsumeDataAfterRestart) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);
    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    close(effect);
    destroy(mFactory, effect);
}

// Send data to IDLE effects and expect it to be consumed after effect start.
TEST_P(AudioEffectTest, SendDataAtIdleAndConsumeDataInProcessing) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    close(effect);
    destroy(mFactory, effect);
}

// Send data multiple times.
TEST_P(AudioEffectTest, ProcessDataMultipleTimes) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);

    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);
    // expect no status and data after consume
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);
    // expect no status and data after consume
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    close(effect);
    destroy(mFactory, effect);
}

// Send data to IDLE state effects and expect it not be consumed.
TEST_P(AudioEffectTest, NotConsumeDataInIdleState) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    command(effect, CommandId::START);
    expectState(effect, State::PROCESSING);
    EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer);

    command(effect, CommandId::STOP);
    expectState(effect, State::IDLE);

    close(effect);
    destroy(mFactory, effect);
}

// Send data to closed effects and expect it not be consumed.
TEST_P(AudioEffectTest, NotConsumeDataByClosedEffect) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, std::nullopt /* specific */, &ret, EX_NONE);
    close(effect);

    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);

    std::vector<float> buffer;
    EffectHelper::allocateInputData(common, inputMQ, buffer);
    EffectHelper::writeToFmq(inputMQ, buffer);
    EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer);

    destroy(mFactory, effect);
}

// Send data to multiple effects.
TEST_P(AudioEffectTest, ConsumeDataMultipleEffects) {
    std::shared_ptr<IEffect> effect1, effect2;
    create(mFactory, effect1, mIdentity);
    create(mFactory, effect2, mIdentity);

    Parameter::Common common1 = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    Parameter::Common common2 = EffectHelper::createParamCommon(
            1 /* session */, 1 /* ioHandle */, 48000 /* iSampleRate */, 48000 /* oSampleRate */,
            2 * kInputFrameCount /* iFrameCount */, 2 * kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret1, ret2;
    open(effect1, common1, std::nullopt /* specific */, &ret1, EX_NONE);
    open(effect2, common2, std::nullopt /* specific */, &ret2, EX_NONE);
    command(effect1, CommandId::START);
    expectState(effect1, State::PROCESSING);
    command(effect2, CommandId::START);
    expectState(effect2, State::PROCESSING);

    auto statusMQ1 = std::make_unique<EffectHelper::StatusMQ>(ret1.statusMQ);
    auto inputMQ1 = std::make_unique<EffectHelper::DataMQ>(ret1.inputDataMQ);
    auto outputMQ1 = std::make_unique<EffectHelper::DataMQ>(ret1.outputDataMQ);

    std::vector<float> buffer1, buffer2;
    EffectHelper::allocateInputData(common1, inputMQ1, buffer1);
    EffectHelper::writeToFmq(inputMQ1, buffer1);
    EffectHelper::readFromFmq(statusMQ1, 1, outputMQ1, buffer1.size(), buffer1);

    auto statusMQ2 = std::make_unique<EffectHelper::StatusMQ>(ret2.statusMQ);
    auto inputMQ2 = std::make_unique<EffectHelper::DataMQ>(ret2.inputDataMQ);
    auto outputMQ2 = std::make_unique<EffectHelper::DataMQ>(ret2.outputDataMQ);
    EffectHelper::allocateInputData(common2, inputMQ2, buffer2);
    EffectHelper::writeToFmq(inputMQ2, buffer2);
    EffectHelper::readFromFmq(statusMQ2, 1, outputMQ2, buffer2.size(), buffer2);

    command(effect1, CommandId::STOP);
    expectState(effect1, State::IDLE);
    close(effect1);
    destroy(mFactory, effect1);

    command(effect2, CommandId::STOP);
    expectState(effect2, State::IDLE);
    close(effect2);
    destroy(mFactory, effect2);
}

INSTANTIATE_TEST_SUITE_P(
        SingleEffectInstanceTest, AudioEffectTest,
        ::testing::Combine(testing::ValuesIn(
                EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor))),
        [](const testing::TestParamInfo<AudioEffectTest::ParamType>& info) {
            auto msSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
            auto instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::ostringstream address;
            address << msSinceEpoch << "_factory_" << instance.first.get();
            std::string name = address.str() + "_UUID_timeLow_" +
                               ::android::internal::ToString(instance.second.uuid.timeLow) +
                               "_timeMid_" +
                               ::android::internal::ToString(instance.second.uuid.timeMid);
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            std::cout << name << " xxx \n";
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
