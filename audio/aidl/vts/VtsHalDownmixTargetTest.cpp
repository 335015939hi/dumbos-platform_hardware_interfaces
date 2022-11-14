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

#define LOG_TAG "VtsHalDownmixTargetTest"

#include <Utils.h>
#include <aidl/Vintf.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Downmix;
using aidl::android::hardware::audio::effect::DownmixTypeUUID;
using aidl::android::hardware::audio::effect::EffectNullUuid;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_TYPE };
using DownmixParamTestParam = std::tuple<std::string, Downmix::Type>;

// Testing for enum values
const std::vector<Downmix::Type> kTypeValues = {Downmix::Type::STRIP, Downmix::Type::FOLD};

class DownmixParamTest : public ::testing::TestWithParam<DownmixParamTestParam>,
                         public EffectHelper {
  public:
    DownmixParamTest()
        : EffectHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())),
          mParamType(std::get<PARAM_TYPE>(GetParam())) {}

    void SetUp() override {
        CreateEffectsWithUUID(DownmixTypeUUID);
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
        OpenEffects(DownmixTypeUUID);
        SCOPED_TRACE(testing::Message() << "type: " << static_cast<int>(mParamType));
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
        CleanUp();
    }

    Downmix::Type mParamType = Downmix::Type::STRIP;

    void SetAndGetDownmixParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTags) {
                auto& tag = it.first;
                auto& dm = it.second;

                // set parameter
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::downmix>(dm);
                expectParam.set<Parameter::specific>(specific);
                // All values are valid, set parameter should succeed
                EXPECT_STATUS(EX_NONE, effect->setParameter(expectParam)) << expectParam.toString();

                // get parameter
                Parameter getParam;
                Parameter::Id id;
                Downmix::Id dmId;
                dmId.set<Downmix::Id::commonTag>(tag);
                id.set<Parameter::Id::downmixTag>(dmId);
                EXPECT_STATUS(EX_NONE, effect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam);
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
    }

    void addTypeParam(Downmix::Type type) {
        Downmix dm;
        dm.set<Downmix::type>(type);
        mTags.push_back({Downmix::type, dm});
    }

  private:
    std::vector<std::pair<Downmix::Tag, Downmix>> mTags;

    void initParamSpecific() {
        Downmix dm;
        dm.set<Downmix::type>(Downmix::Type::STRIP);
        Parameter::Specific specific;
        specific.set<Parameter::Specific::downmix>(dm);
        setSpecific(specific);
    }

    void CleanUp() { mTags.clear(); }
};

TEST_P(DownmixParamTest, SetAndGetType) {
    EXPECT_NO_FATAL_FAILURE(addTypeParam(mParamType));
    SetAndGetDownmixParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DownmixTest, DownmixParamTest,
        ::testing::Combine(
                testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                testing::ValuesIn(kTypeValues)),
        [](const testing::TestParamInfo<DownmixParamTest::ParamType>& info) {
            std::string instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::string type = std::to_string(static_cast<int>(std::get<PARAM_TYPE>(info.param)));

            std::string name = instance + "_type" + type;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DownmixParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
