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

#define LOG_TAG "VtsHalReverbTest"

#include <Utils.h>
#include <aidl/Vintf.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kEnvReverbTypeUUID;
using aidl::android::hardware::audio::effect::kPresetReverbTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Reverb;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * This range is verified with IEffect.getDescriptor() and range defined in the documentation, for
 * any index supported value test expects EX_NONE from IEffect.setParameter(), otherwise expects
 * EX_ILLEGAL_ARGUMENT.
 */
const std::vector<int> kRoomLevelValues = {Reverb::MIN_ROOM_LEVEL_MB - 1, Reverb::MIN_ROOM_LEVEL_MB,
                                           Reverb::MAX_ROOM_LEVEL_MB,
                                           Reverb::MAX_ROOM_LEVEL_MB + 1};
const std::vector<int> kRoomHfLevelValues = {
        Reverb::MIN_ROOM_HF_LEVEL_MB - 1, Reverb::MIN_ROOM_HF_LEVEL_MB,
        Reverb::MAX_ROOM_HF_LEVEL_MB, Reverb::MAX_ROOM_HF_LEVEL_MB + 1};
const std::vector<int> kDecayTimeValues = {Reverb::MIN_DECAY_TIME_MS - 1, Reverb::MIN_DECAY_TIME_MS,
                                           Reverb::MAX_DECAY_TIME_MS,
                                           Reverb::MAX_DECAY_TIME_MS + 1};
const std::vector<int> kDecayHfRatioValues = {
        Reverb::MIN_DECAY_HF_RATIO_PM - 1, Reverb::MIN_DECAY_HF_RATIO_PM,
        Reverb::MAX_DECAY_HF_RATIO_PM, Reverb::MAX_DECAY_HF_RATIO_PM + 1};
const std::vector<int> kLevelValues = {Reverb::MIN_LEVEL_MB - 1, Reverb::MIN_LEVEL_MB,
                                       Reverb::MAX_LEVEL_MB, Reverb::MAX_LEVEL_MB + 1};
const std::vector<int> kDelayValues = {Reverb::MIN_DELAY_MS - 1, Reverb::MIN_DELAY_MS,
                                       Reverb::MAX_DELAY_MS, Reverb::MAX_DELAY_MS + 1};
const std::vector<int> kDiffusionValues = {Reverb::MIN_DIFFUSION_PM - 1, Reverb::MIN_DIFFUSION_PM,
                                           Reverb::MAX_DIFFUSION_PM, Reverb::MAX_DIFFUSION_PM + 1};
const std::vector<int> kDensityValues = {Reverb::MIN_DENSITY_PM - 1, Reverb::MIN_DENSITY_PM,
                                         Reverb::MAX_DENSITY_PM, Reverb::MAX_DENSITY_PM + 1};

const std::vector<AudioUuid> kReverbTypeUuids = {kEnvReverbTypeUUID, kPresetReverbTypeUUID};

class ReverbHelper : public EffectHelper {
  public:
    ReverbHelper(std::pair<std::shared_ptr<IFactory>, Descriptor::Identity> pair) {
        std::tie(mFactory, mIdentity) = pair;
    }

    void SetUpReverb() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mIdentity));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownReverb() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        Reverb rv = Reverb::make<Reverb::roomLevelMb>(Reverb::MIN_ROOM_LEVEL_MB);
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::reverb>(rv);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor::Identity mIdentity;
    int mRoomLevel = Reverb::MIN_ROOM_LEVEL_MB;
    int mRoomHfLevel = Reverb::MAX_ROOM_HF_LEVEL_MB;
    int mDecayTime = 1000;
    int mDecayHfRatio = 500;
    int mLevel = Reverb::MIN_LEVEL_MB;
    int mDelay = 40;
    int mDiffusion = Reverb::MAX_DIFFUSION_PM;
    int mDensity = Reverb::MAX_DENSITY_PM;
    bool mBypass = false;

    void SetAndGetReverbParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& rv = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isTagInRange(it.first, it.second, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::reverb>(rv);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                Reverb::Id rvId;
                rvId.set<Reverb::Id::commonTag>(tag);
                id.set<Parameter::Id::reverbTag>(rvId);
                // if set success, then get should match
                EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
                EXPECT_EQ(expectParam, getParam);
            }
        }
    }

    void addRoomLevelParam() {
        Reverb rv;
        rv.set<Reverb::roomLevelMb>(mRoomLevel);
        mTags.push_back({Reverb::roomLevelMb, rv});
    }

    void addRoomHfLevelParam(int roomHfLevel) {
        Reverb rv;
        rv.set<Reverb::roomHfLevelMb>(roomHfLevel);
        mTags.push_back({Reverb::roomHfLevelMb, rv});
    }

    void addDecayTimeParam(int decayTime) {
        Reverb rv;
        rv.set<Reverb::decayTimeMs>(decayTime);
        mTags.push_back({Reverb::decayTimeMs, rv});
    }

    void addDecayHfRatioParam(int decayHfRatio) {
        Reverb rv;
        rv.set<Reverb::decayHfRatioPm>(decayHfRatio);
        mTags.push_back({Reverb::decayHfRatioPm, rv});
    }

    void addLevelParam(int level) {
        Reverb rv;
        rv.set<Reverb::levelMb>(level);
        mTags.push_back({Reverb::levelMb, rv});
    }

    void addDelayParam(int delay) {
        Reverb rv;
        rv.set<Reverb::delayMs>(delay);
        mTags.push_back({Reverb::delayMs, rv});
    }

    void addDiffusionParam(int diffusion) {
        Reverb rv;
        rv.set<Reverb::diffusionPm>(diffusion);
        mTags.push_back({Reverb::diffusionPm, rv});
    }

    void addDensityParam(int density) {
        Reverb rv;
        rv.set<Reverb::densityPm>(density);
        mTags.push_back({Reverb::densityPm, rv});
    }

    void addBypassParam(bool bypass) {
        Reverb rv;
        rv.set<Reverb::bypass>(bypass);
        mTags.push_back({Reverb::bypass, rv});
    }

    bool isTagInRange(const Reverb::Tag& tag, const Reverb rv, const Descriptor& desc) const {
        const Reverb::Capability& rvCap = desc.capability.get<Capability::reverb>();
        switch (tag) {
            case Reverb::roomLevelMb: {
                int roomLevel = rv.get<Reverb::roomLevelMb>();
                return isRoomLevelInRange(roomLevel);
            }
            case Reverb::roomHfLevelMb: {
                int roomHfLevel = rv.get<Reverb::roomHfLevelMb>();
                return isRoomHfLevelInRange(roomHfLevel);
            }
            case Reverb::decayTimeMs: {
                int decayTime = rv.get<Reverb::decayTimeMs>();
                return isDecayTimeInRange(rvCap, decayTime);
            }
            case Reverb::decayHfRatioPm: {
                int decayHfRatio = rv.get<Reverb::decayHfRatioPm>();
                return isDecayHfRatioInRange(decayHfRatio);
            }
            case Reverb::levelMb: {
                int level = rv.get<Reverb::levelMb>();
                return isLevelInRange(level);
            }
            case Reverb::delayMs: {
                int delay = rv.get<Reverb::delayMs>();
                return isDelayInRange(delay);
            }
            case Reverb::diffusionPm: {
                int diffusion = rv.get<Reverb::diffusionPm>();
                return isDiffusionInRange(diffusion);
            }
            case Reverb::densityPm: {
                int density = rv.get<Reverb::densityPm>();
                return isDensityInRange(density);
            }
            case Reverb::bypass: {
                return true;
            }
            default:
                return false;
        }
        return false;
    }

    bool isRoomLevelInRange(int roomLevel) const {
        return roomLevel >= Reverb::MIN_ROOM_LEVEL_MB && roomLevel <= Reverb::MAX_ROOM_LEVEL_MB;
    }

    bool isRoomHfLevelInRange(int roomHfLevel) const {
        return roomHfLevel >= Reverb::MIN_ROOM_HF_LEVEL_MB &&
               roomHfLevel <= Reverb::MAX_ROOM_HF_LEVEL_MB;
    }

    bool isDecayTimeInRange(const Reverb::Capability& cap, int decayTime) const {
        return decayTime >= Reverb::MIN_DECAY_TIME_MS && decayTime <= Reverb::MAX_DECAY_TIME_MS &&
               decayTime <= cap.maxDecayTimeMs;
    }

    bool isDecayHfRatioInRange(int decayHfRatio) const {
        return decayHfRatio >= Reverb::MIN_DECAY_HF_RATIO_PM &&
               decayHfRatio <= Reverb::MAX_DECAY_HF_RATIO_PM;
    }

    bool isLevelInRange(int level) const {
        return level >= Reverb::MIN_LEVEL_MB && level <= Reverb::MAX_LEVEL_MB;
    }

    bool isDelayInRange(int delay) const {
        return delay >= Reverb::MIN_DELAY_MS && delay <= Reverb::MAX_DELAY_MS;
    }

    bool isDiffusionInRange(int diffusion) const {
        return diffusion >= Reverb::MIN_DIFFUSION_PM && diffusion <= Reverb::MAX_DIFFUSION_PM;
    }

    bool isDensityInRange(int density) const {
        return density >= Reverb::MIN_DENSITY_PM && density <= Reverb::MAX_DENSITY_PM;
    }

  private:
    std::vector<std::pair<Reverb::Tag, Reverb>> mTags;
    void CleanUp() { mTags.clear(); }
};

class ReverbRoomLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbRoomLevelTest() : ReverbHelper(std::get<0>(GetParam())) {
        mRoomLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbRoomLevelTest, SetAndGetRoomLevel) {
    EXPECT_NO_FATAL_FAILURE(addRoomLevelParam());
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbRoomLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kRoomLevelValues)),
        [](const testing::TestParamInfo<ReverbRoomLevelTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string roomLevel = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_roomLevel" + roomLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbRoomHfLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbRoomHfLevelTest() : ReverbHelper(std::get<0>(GetParam())) {
        mRoomHfLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbRoomHfLevelTest, SetAndGetRoomHfLevel) {
    EXPECT_NO_FATAL_FAILURE(addRoomHfLevelParam(mRoomHfLevel));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbRoomHfLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kRoomHfLevelValues)),
        [](const testing::TestParamInfo<ReverbRoomHfLevelTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string roomHfLevel = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_roomHfLevel" + roomHfLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbDecayTimeTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbDecayTimeTest() : ReverbHelper(std::get<0>(GetParam())) {
        mDecayTime = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbDecayTimeTest, SetAndGetDecayTime) {
    EXPECT_NO_FATAL_FAILURE(addDecayTimeParam(mDecayTime));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbDecayTimeTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kDecayTimeValues)),
        [](const testing::TestParamInfo<ReverbDecayTimeTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string decayTime = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_decayTime" + decayTime;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbDecayHfRatioTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbDecayHfRatioTest() : ReverbHelper(std::get<0>(GetParam())) {
        mDecayHfRatio = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbDecayHfRatioTest, SetAndGetDecayHfRatio) {
    EXPECT_NO_FATAL_FAILURE(addDecayHfRatioParam(mDecayHfRatio));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbDecayHfRatioTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kDecayHfRatioValues)),
        [](const testing::TestParamInfo<ReverbDecayHfRatioTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string decayHfRatio = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_decayHfRatio" + decayHfRatio;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbLevelTest() : ReverbHelper(std::get<0>(GetParam())) { mLevel = std::get<1>(GetParam()); }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbLevelTest, SetAndGetLevel) {
    EXPECT_NO_FATAL_FAILURE(addLevelParam(mLevel));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kLevelValues)),
        [](const testing::TestParamInfo<ReverbDecayHfRatioTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string level = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_level" + level;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbDelayTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbDelayTest() : ReverbHelper(std::get<0>(GetParam())) { mDelay = std::get<1>(GetParam()); }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbDelayTest, SetAndGetDelay) {
    EXPECT_NO_FATAL_FAILURE(addDelayParam(mDelay));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbDelayTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kDelayValues)),
        [](const testing::TestParamInfo<ReverbDelayTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string delay = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_delay" + delay;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbDiffusionTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbDiffusionTest() : ReverbHelper(std::get<0>(GetParam())) {
        mDiffusion = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbDiffusionTest, SetAndGetDiffusion) {
    EXPECT_NO_FATAL_FAILURE(addDiffusionParam(mDiffusion));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbDiffusionTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kDiffusionValues)),
        [](const testing::TestParamInfo<ReverbDiffusionTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string diffusion = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_diffusion" + diffusion;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbDensityTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int>>,
      public ReverbHelper {
  public:
    ReverbDensityTest() : ReverbHelper(std::get<0>(GetParam())) {
        mDensity = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbDensityTest, SetAndGetDensity) {
    EXPECT_NO_FATAL_FAILURE(addDensityParam(mDensity));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbDensityTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::ValuesIn(kDensityValues)),
        [](const testing::TestParamInfo<ReverbDensityTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string density = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_density" + density;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

class ReverbBypassTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, bool>>,
      public ReverbHelper {
  public:
    ReverbBypassTest() : ReverbHelper(std::get<0>(GetParam())) {
        mBypass = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(ReverbBypassTest, SetAndGetBypass) {
    EXPECT_NO_FATAL_FAILURE(addBypassParam(mBypass));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        ReverbTest, ReverbBypassTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kReverbTypeUuids)),
                           testing::Bool()),
        [](const testing::TestParamInfo<ReverbBypassTest::ParamType>& info) {
            auto instance = std::get<0>(info.param);
            std::string bypass = std::to_string(std::get<1>(info.param));

            std::string name = instance.second.uuid.toString() + "_bypass" + bypass;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
