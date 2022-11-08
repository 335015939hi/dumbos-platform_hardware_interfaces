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

#include <aidl/Vintf.h>

#define LOG_TAG "VtsHalBassBoostTest"
#include <Utils.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::BassBoost;
using aidl::android::hardware::audio::effect::BassBoostTypeUUID;
using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_STRENGTH };
using BassBoostParamTestParam = std::tuple<std::string, int>;

/*
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * Parameter should be within the valid range defined in the documentation,
 * for any supported value test expects EX_NONE from IEffect.setParameter(),
 * otherwise expect EX_ILLEGAL_ARGUMENT.
 */
constexpr std::pair<int, int> kStrengthRange = {-1, 1002};  // valid range [0, 1000]

class BassBoostParamTest : public ::testing::TestWithParam<BassBoostParamTestParam>,
                           public EffectHelper {
  public:
    BassBoostParamTest()
        : EffectHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())),
          mParamStrength(std::get<PARAM_STRENGTH>(GetParam())) {}

    void SetUp() override {
        CreateEffectsWithUUID(BassBoostTypeUUID);
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
        OpenEffects(BassBoostTypeUUID);
        SCOPED_TRACE(testing::Message() << "strength: " << mParamStrength);
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
        CleanUp();
    }

    int mParamStrength = 0;

    void SetAndGetBassBoostParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTags) {
                auto& tag = it.first;
                auto& bb = it.second;

                // validate parameter
                Descriptor desc;
                ASSERT_STATUS(EX_NONE, effect->getDescriptor(&desc));
                const bool valid = isTagInRange(it.first, it.second, desc);
                const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

                // set parameter
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::bassBoost>(bb);
                expectParam.set<Parameter::specific>(specific);
                EXPECT_STATUS(expected, effect->setParameter(expectParam))
                        << expectParam.toString();

                // only get if parameter in range and set success
                if (expected == EX_NONE) {
                    Parameter getParam;
                    Parameter::Id id;
                    BassBoost::Id bbId;
                    bbId.set<BassBoost::Id::commonTag>(tag);
                    id.set<Parameter::Id::bassBoostTag>(bbId);
                    // if set success, then get should match
                    EXPECT_STATUS(expected, effect->getParameter(id, &getParam));
                    EXPECT_EQ(expectParam, getParam);
                }
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
    }

    void addStrengthParam(int strength) {
        BassBoost bb;
        bb.set<BassBoost::strengthPm>(strength);
        mTags.push_back({BassBoost::strengthPm, bb});
    }

    bool isTagInRange(const BassBoost::Tag& tag, const BassBoost& bb,
                      const Descriptor& desc) const {
        std::cout << "xxx" << toString(tag) << " " << desc.toString();
        const BassBoost::Capability& bbCap = desc.capability.get<Capability::bassBoost>();
        switch (tag) {
            case BassBoost::strengthPm: {
                int strength = bb.get<BassBoost::strengthPm>();
                return isStrengthInRange(bbCap, strength);
            }
            default:
                return false;
        }
        return false;
    }

    bool isStrengthInRange(const BassBoost::Capability& cap, int strength) const {
        return cap.strengthSupported && strength >= 0 && strength <= 1000;
    }

  private:
    std::vector<std::pair<BassBoost::Tag, BassBoost>> mTags;

    void initParamSpecific() {
        BassBoost bb;
        bb.set<BassBoost::strengthPm>(0);
        Parameter::Specific specific;
        specific.set<Parameter::Specific::bassBoost>(bb);
        setSpecific(specific);
    }

    void CleanUp() { mTags.clear(); }
};

TEST_P(BassBoostParamTest, SetAndGetStrength) {
    EXPECT_NO_FATAL_FAILURE(addStrengthParam(mParamStrength));
    SetAndGetBassBoostParameters();
}

INSTANTIATE_TEST_SUITE_P(
        BassBoostTest, BassBoostParamTest,
        ::testing::Combine(
                testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                testing::Range(kStrengthRange.first, kStrengthRange.second)),
        [](const testing::TestParamInfo<BassBoostParamTest::ParamType>& info) {
            std::string instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::string strength = std::to_string(std::get<PARAM_STRENGTH>(info.param));

            std::string name = instance + "_strength" + strength;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BassBoostParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
