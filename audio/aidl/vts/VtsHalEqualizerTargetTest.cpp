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

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define LOG_TAG "VtsHalEqualizerTest"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectHelper.h"
#include "TestUtils.h"
#include "effect-impl/EffectUUID.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kEqualizerTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEfectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_PRESET_INDEX, PARAM_BAND_INDEX, PARAM_BAND_LEVEL };
using EqualizerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor::Identity>, int, int, int>;

/*
Testing parameter range, assuming the parameter supported by effect is in this range.
This range is verified with IEffect.getDescriptor(), for any index supported vts expect EX_NONE
from IEffect.setParameter(), otherwise expect EX_ILLEGAL_ARGUMENT.
*/
constexpr std::pair<int, int> kPresetIndexRange = {-1, 10};  // valid range [0, 9]
constexpr std::pair<int, int> kBandIndexRange = {-1, 5};     // valid range [0, 4]
const std::vector<int> kBandLevels = {0, -10, 10};           // needs update with implementation

class EqualizerTest : public ::testing::TestWithParam<EqualizerParamTestParam>,
                      public EffectHelper {
  public:
    EqualizerTest()
        : mParamPresetIndex(std::get<PARAM_PRESET_INDEX>(GetParam())),
          mParamBandIndex(std::get<PARAM_BAND_INDEX>(GetParam())),
          mParamBandLevel(std::get<PARAM_BAND_LEVEL>(GetParam())) {
        std::tie(mFactory, mIdentity) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {}

    void TearDown() override {}

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    Descriptor::Identity mIdentity;
    int mParamPresetIndex = 0;
    int mParamBandIndex = 0;
    int mParamBandLevel = 0;

    void SetAndGetEqualizerParameters(std::shared_ptr<IEffect>& effect) {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& eq = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, effect->getDescriptor(&desc));
            const bool valid = isTagInRange(it.first, it.second, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::equalizer>(eq);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, effect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                Equalizer::Id eqId;
                eqId.set<Equalizer::Id::commonTag>(tag);
                id.set<Parameter::Id::equalizerTag>(eqId);
                // if set success, then get should match
                EXPECT_STATUS(expected, effect->getParameter(id, &getParam));
                EXPECT_TRUE(isEqParameterExpected(expectParam, getParam));
            }
        }
    }

    bool isEqParameterExpected(const Parameter& expect, const Parameter& target) {
        // if parameter same, then for sure they are matched
        if (expect == target) return true;

        // if not, see if target include the expect parameter, and others all default (0).
        /*
         * This is to verify the case of client setParameter to a single bandLevel ({3, -1} for
         * example), and return of getParameter must be [{0, 0}, {1, 0}, {2, 0}, {3, -1}, {4, 0}]
         */
        EXPECT_EQ(expect.getTag(), Parameter::specific);
        EXPECT_EQ(target.getTag(), Parameter::specific);

        Parameter::Specific expectSpec = expect.get<Parameter::specific>(),
                            targetSpec = target.get<Parameter::specific>();
        EXPECT_EQ(expectSpec.getTag(), Parameter::Specific::equalizer);
        EXPECT_EQ(targetSpec.getTag(), Parameter::Specific::equalizer);

        Equalizer expectEq = expectSpec.get<Parameter::Specific::equalizer>(),
                  targetEq = targetSpec.get<Parameter::Specific::equalizer>();
        EXPECT_EQ(expectEq.getTag(), targetEq.getTag());

        auto eqTag = targetEq.getTag();
        switch (eqTag) {
            case Equalizer::bandLevels: {
                auto expectBl = expectEq.get<Equalizer::bandLevels>();
                std::sort(expectBl.begin(), expectBl.end(),
                          [](const auto& a, const auto& b) { return a.index < b.index; });
                auto targetBl = targetEq.get<Equalizer::bandLevels>();
                if (!std::includes(targetBl.begin(), targetBl.end(), expectBl.begin(),
                                   expectBl.end())) {
                    for (auto it : targetBl) {
                        std::cout << it.toString() << ", ";
                    }
                    std::cout << "\n";
                    for (auto it : expectBl) {
                        std::cout << it.toString() << ", ";
                    }
                    return false;
                }
                return true;
            }
            default:
                return false;
        }
        return false;
    }

    void addPresetParam(int preset) {
        Equalizer eq;
        eq.set<Equalizer::preset>(preset);
        mTags.push_back({Equalizer::preset, eq});
    }

    void addBandLevelsParam(std::vector<Equalizer::BandLevel>& bandLevels) {
        Equalizer eq;
        eq.set<Equalizer::bandLevels>(bandLevels);
        mTags.push_back({Equalizer::bandLevels, eq});
    }

    bool isTagInRange(const Equalizer::Tag& tag, const std::unique_ptr<Equalizer>& eq,
                      const Descriptor& desc) const {
        const Equalizer::Capability& eqCap = desc.capability.get<Capability::equalizer>();
        switch (tag) {
            case Equalizer::preset: {
                int index = eq->get<Equalizer::preset>();
                return isPresetIndexInRange(eqCap, index);
            }
            case Equalizer::bandLevels: {
                auto& bandLevel = eq->get<Equalizer::bandLevels>();
                return isBandIndexInRange(eqCap, bandLevel);
            }
            default:
                return false;
        }
        return false;
    }

    bool isPresetIndexInRange(const Equalizer::Capability& cap, int idx) const {
        const auto [min, max] =
                std::minmax_element(cap.presets.begin(), cap.presets.end(),
                                    [](const auto& a, const auto& b) { return a.index < b.index; });
        return idx >= min->index && idx <= max->index;
    }

    bool isBandIndexInRange(const Equalizer::Capability& cap,
                            const std::vector<Equalizer::BandLevel>& bandLevel) const {
        for (auto& it : bandLevel) {
            if (!isBandIndexInRange(cap, it.index)) return false;
        }
        return true;
    }

    bool isBandIndexInRange(const Equalizer::Capability& cap, int idx) const {
        const auto [min, max] =
                std::minmax_element(cap.bandFrequencies.begin(), cap.bandFrequencies.end(),
                                    [](const auto& a, const auto& b) { return a.index < b.index; });
        return idx >= min->index && idx <= max->index;
    }

    Parameter::Specific getDefaultParamSpecific() {
        Equalizer eq = Equalizer::make<Equalizer::preset>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::equalizer>(eq);
        return specific;
    }

  private:
    std::vector<std::pair<Equalizer::Tag, Equalizer>> mTags;

    bool validCapabilityTag(Capability& cap) { return cap.getTag() == Capability::equalizer; }

    void CleanUp() { mTags.clear(); }
};

TEST_P(EqualizerTest, SetAndGetPreset) {
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Specific specific = getDefaultParamSpecific();
    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, specific, &ret, EX_NONE);
    EXPECT_NO_FATAL_FAILURE(addPresetParam(mParamPresetIndex));
    SetAndGetEqualizerParameters(effect);
    close(effect);
    destroy(mFactory, effect);
}

TEST_P(EqualizerTest, SetAndGetSingleBand) {
    std::vector<Equalizer::BandLevel> bandLevels{{mParamBandIndex, mParamBandLevel}};
    EXPECT_NO_FATAL_FAILURE(addBandLevelsParam(bandLevels));
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Specific specific = getDefaultParamSpecific();
    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, specific, &ret, EX_NONE);
    EXPECT_NO_FATAL_FAILURE(addPresetParam(mParamPresetIndex));
    SetAndGetEqualizerParameters(effect);
    close(effect);
    destroy(mFactory, effect);
}

TEST_P(EqualizerTest, SetAndGetMultiBands) {
    std::vector<Equalizer::BandLevel> bandLevels{{mParamBandIndex, mParamBandLevel},
                                                 {mParamBandIndex - 1, mParamBandLevel - 1},
                                                 {mParamBandIndex + 1, mParamBandLevel + 1}};
    EXPECT_NO_FATAL_FAILURE(addBandLevelsParam(bandLevels));
    std::shared_ptr<IEffect> effect;
    create(mFactory, effect, mIdentity);

    Parameter::Specific specific = getDefaultParamSpecific();
    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    open(effect, common, specific, &ret, EX_NONE);
    EXPECT_NO_FATAL_FAILURE(addPresetParam(mParamPresetIndex));
    SetAndGetEqualizerParameters(effect);
    close(effect);
    destroy(mFactory, effect);
}

INSTANTIATE_TEST_SUITE_P(
        EqualizerTest, EqualizerTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEqualizerTypeUUID)),
                           testing::Range(kPresetIndexRange.first, kPresetIndexRange.second),
                           testing::Range(kBandIndexRange.first, kBandIndexRange.second),
                           testing::ValuesIn(kBandLevels)),
        [](const testing::TestParamInfo<EqualizerTest::ParamType>& info) {
            auto msSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
            auto instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::string presetIdx = std::to_string(std::get<PARAM_PRESET_INDEX>(info.param));
            std::string bandIdx = std::to_string(std::get<PARAM_BAND_INDEX>(info.param));
            std::string bandLevel = std::to_string(std::get<PARAM_BAND_LEVEL>(info.param));
            std::ostringstream address;
            address << msSinceEpoch << "_factory_" << instance.first.get();
            std::string name =
                    address.str() + "_UUID_timeLow_" +
                    ::android::internal::ToString(instance.second.uuid.timeLow) + "_timeMid_" +
                    ::android::internal::ToString(instance.second.uuid.timeMid) + "_presetIndex_" +
                    presetIdx + "_bandIndex_" + bandIdx + "_bandLevel_" + bandLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            std::cout << name << " xxx \n";
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
