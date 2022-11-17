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
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Reverb;
using aidl::android::hardware::audio::effect::ReverbTypeUUID;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
const int MIN_LEVEL = -6000;
const int MAX_LEVEL = 0;
const int MIN_ROOM_HF_LEVEL = -4000;
const int MAX_ROOM_HF_LEVEL = 0;
const int MIN_DECAY_TIME = 100;
const int MAX_DECAY_TIME = 20000;
const int MIN_DECAY_HF_RATIO = 100;
const int MAX_DECAY_HF_RATIO = 1000;
const int MIN_DELAY = 0;
const int MAX_DELAY = 65;
const int MIN_DIFFUSION = 0;
const int MAX_DIFFUSION = 1000;
const int MIN_DENSITY = 0;
const int MAX_DENSITY = 1000;

/**
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * This range is verified with IEffect.getDescriptor() and range defined in the documentation, for
 * any index supported value test expects EX_NONE from IEffect.setParameter(), otherwise expects
 * EX_ILLEGAL_ARGUMENT.
 */
const std::vector<int> kLevelValues = {MIN_LEVEL - 1, MIN_LEVEL, MAX_LEVEL, MAX_LEVEL + 1};
const std::vector<int> kRoomHfLevelValues = {MIN_ROOM_HF_LEVEL - 1, MIN_ROOM_HF_LEVEL,
                                             MAX_ROOM_HF_LEVEL, MAX_ROOM_HF_LEVEL + 1};
const std::vector<int> kDecayTimeValues = {MIN_DECAY_TIME - 1, MIN_DECAY_TIME, MAX_DECAY_TIME,
                                           MAX_DECAY_TIME + 1};
const std::vector<int> kDecayHfRatioValues = {MIN_DECAY_HF_RATIO - 1, MIN_DECAY_HF_RATIO,
                                              MAX_DECAY_HF_RATIO, MAX_DECAY_HF_RATIO + 1};
const std::vector<int> kDelayValues = {MIN_DELAY - 1, MIN_DELAY, MAX_DELAY, MAX_DELAY + 1};
const std::vector<int> kDiffusionValues = {MIN_DIFFUSION - 1, MIN_DIFFUSION, MAX_DIFFUSION,
                                           MAX_DIFFUSION + 1};
const std::vector<int> kDensityValues = {MIN_DENSITY - 1, MIN_DENSITY, MAX_DENSITY,
                                         MAX_DENSITY + 1};

class ReverbHelper : public EffectHelper {
  public:
    ReverbHelper(std::string instanceName) : EffectHelper(instanceName) {}

    void SetUpReverb() {
        CreateEffectsWithUUID(ReverbTypeUUID);
        initParamCommonFormat();
        initParamCommon();
        OpenEffects(ReverbTypeUUID);
        SCOPED_TRACE(testing::Message()
                     << " roomLevel: " << mRoomLevel << " roomHfLevel: " << mRoomHfLevel
                     << " decayTime: " << mDecayTime << " decayHfRatio: " << mDecayHfRatio
                     << " level: " << mLevel << " delay: " << mDelay << " diffusion: " << mDiffusion
                     << " density: " << mDensity << " bypass: " << mBypass);
    }

    void TearDownReverb() {
        CloseEffects();
        DestroyEffects();
        CleanUp();
    }

    int mRoomLevel = MIN_LEVEL;
    int mRoomHfLevel = MAX_ROOM_HF_LEVEL;
    int mDecayTime = 1000;
    int mDecayHfRatio = 500;
    int mLevel = MIN_LEVEL;
    int mDelay = 40;
    int mDiffusion = MAX_DIFFUSION;
    int mDensity = MAX_DENSITY;
    bool mBypass = false;

    void SetAndGetReverbParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTags) {
                auto& tag = it.first;
                auto& rv = it.second;

                // validate parameter
                Descriptor desc;
                ASSERT_STATUS(EX_NONE, effect->getDescriptor(&desc));
                const bool valid = isTagInRange(it.first, it.second, desc);
                const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

                // set
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::reverb>(rv);
                expectParam.set<Parameter::specific>(specific);
                EXPECT_STATUS(expected, effect->setParameter(expectParam))
                        << expectParam.toString();

                // only get if parameter in range and set success
                if (expected == EX_NONE) {
                    Parameter getParam;
                    Parameter::Id id;
                    Reverb::Id rvId;
                    rvId.set<Reverb::Id::commonTag>(tag);
                    id.set<Parameter::Id::reverbTag>(rvId);
                    // if set success, then get should match
                    EXPECT_STATUS(expected, effect->getParameter(id, &getParam));
                    EXPECT_EQ(expectParam, getParam);
                }
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
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
        return roomLevel >= MIN_LEVEL && roomLevel <= MAX_LEVEL;
    }

    bool isRoomHfLevelInRange(int roomHfLevel) const {
        return roomHfLevel >= MIN_ROOM_HF_LEVEL && roomHfLevel <= MAX_ROOM_HF_LEVEL;
    }

    bool isDecayTimeInRange(const Reverb::Capability& cap, int decayTime) const {
        return decayTime >= MIN_DECAY_TIME && decayTime <= MAX_DECAY_TIME &&
               decayTime <= cap.maxDecayTimeMs;
    }

    bool isDecayHfRatioInRange(int decayHfRatio) const {
        return decayHfRatio >= MIN_DECAY_HF_RATIO && decayHfRatio <= MAX_DECAY_HF_RATIO;
    }

    bool isLevelInRange(int level) const { return level >= MIN_LEVEL && level <= MAX_LEVEL; }

    bool isDelayInRange(int delay) const { return delay >= MIN_DELAY && delay <= MAX_DELAY; }

    bool isDiffusionInRange(int diffusion) const {
        return diffusion >= MIN_DIFFUSION && diffusion <= MAX_DIFFUSION;
    }

    bool isDensityInRange(int density) const {
        return density >= MIN_DENSITY && density <= MAX_DENSITY;
    }

  private:
    std::vector<std::pair<Reverb::Tag, Reverb>> mTags;

    void CleanUp() { mTags.clear(); }
};

class ReverbRoomLevelTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbRoomLevelTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kLevelValues)),
                         [](const testing::TestParamInfo<ReverbRoomLevelTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string roomLevel = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_roomLevel" + roomLevel;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbRoomHfLevelTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbRoomHfLevelTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kRoomHfLevelValues)),
                         [](const testing::TestParamInfo<ReverbRoomHfLevelTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string roomHfLevel = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_roomHfLevel" + roomHfLevel;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbDecayTimeTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbDecayTimeTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kDecayTimeValues)),
                         [](const testing::TestParamInfo<ReverbDecayTimeTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string decayTime = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_decayTime" + decayTime;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbDecayHfRatioTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbDecayHfRatioTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kDecayHfRatioValues)),
                         [](const testing::TestParamInfo<ReverbDecayHfRatioTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string decayHfRatio = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_decayHfRatio" + decayHfRatio;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbLevelTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbLevelTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kLevelValues)),
                         [](const testing::TestParamInfo<ReverbDecayHfRatioTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string level = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_level" + level;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbDelayTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbDelayTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kDelayValues)),
                         [](const testing::TestParamInfo<ReverbDelayTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string delay = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_delay" + delay;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbDiffusionTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbDiffusionTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kDiffusionValues)),
                         [](const testing::TestParamInfo<ReverbDiffusionTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string diffusion = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_diffusion" + diffusion;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbDensityTest : public ::testing::TestWithParam<std::tuple<std::string, int>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbDensityTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::ValuesIn(kDensityValues)),
                         [](const testing::TestParamInfo<ReverbDensityTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string density = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_density" + density;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

class ReverbBypassTest : public ::testing::TestWithParam<std::tuple<std::string, bool>>,
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

INSTANTIATE_TEST_SUITE_P(ReverbTest, ReverbBypassTest,
                         ::testing::Combine(testing::ValuesIn(android::getAidlHalInstanceNames(
                                                    IFactory::descriptor)),
                                            testing::Bool()),
                         [](const testing::TestParamInfo<ReverbBypassTest::ParamType>& info) {
                             std::string instance = std::get<0>(info.param);
                             std::string bypass = std::to_string(std::get<1>(info.param));

                             std::string name = instance + "_bypass" + bypass;
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
