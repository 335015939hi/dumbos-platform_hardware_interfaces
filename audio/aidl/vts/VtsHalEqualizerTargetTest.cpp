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

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectHelper.h"
#include "TestUtils.h"
#include "effect-impl/EffectUUID.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::EqualizerTypeUUID;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioUuid;
using aidl::android::media::audio::common::PcmType;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEfectTargetTest.
 */
class EqualizerTest : public testing::TestWithParam<std::string>, public EffectHelper {
  public:
    EqualizerTest() : EffectHelper(GetParam()) {
        setVendor(mVendorExtension);
        setPreset(mPresetIndex);
        setPresets(mPresets);
        setBandLevels(mBandLevels);
        setBandCapabilities(mBandCapability);
    }

    void SetUp() override {
        CreateEffects();
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
        OpenEffects(EqualizerTypeUUID);
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
    }

    void SetAndGetEqualizerParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTagMap) {
                auto& tag = it.first;
                auto& eq = it.second;

                // set
                ::ndk::ScopedAStatus status;
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::equalizer>(*eq.get());
                expectParam.set<Parameter::specific>(specific);
                status = effect->setParameter(expectParam);
                if (tag < minSetTag || tag > maxSetTag) {
                    ASSERT_STATUS(EX_ILLEGAL_ARGUMENT, status);
                } else {
                    ASSERT_STATUS(EX_NONE, status);
                }

                // get
                Parameter getParam;
                Parameter::Specific::Id id;
                id.set<Parameter::Specific::Id::equalizerTag>(tag);
                status = effect->getParameter(id, &getParam);
                if (tag < minGetTag || tag > maxGetTag) {
                    ASSERT_STATUS(EX_ILLEGAL_ARGUMENT, status);
                } else {
                    ASSERT_STATUS(EX_NONE, status);
                    ASSERT_EQ(expectParam, getParam) << "\n"
                                                     << expectParam.toString() << "\n"
                                                     << getParam.toString();
                }
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
    }

    void setVendor(Equalizer::VendorExtension vendor) {
        Equalizer eq;
        eq.set<Equalizer::vendor>(vendor);
        mTagMap[Equalizer::vendor] = std::make_unique<Equalizer>(std::move(eq));
        mVendorExtension = vendor;
    }
    void setPreset(int preset) {
        Equalizer eq;
        eq.set<Equalizer::preset>(preset);
        mTagMap[Equalizer::preset] = std::make_unique<Equalizer>(std::move(eq));
        mPresetIndex = preset;
    }
    void setPresets(std::vector<Equalizer::Preset> presets) {
        Equalizer eq;
        eq.set<Equalizer::presets>(presets);
        mTagMap[Equalizer::presets] = std::make_unique<Equalizer>(std::move(eq));
        mPresets = presets;
    }
    void setBandLevels(std::vector<Equalizer::BandLevel> bandLevels) {
        Equalizer eq;
        eq.set<Equalizer::bandLevels>(bandLevels);
        mTagMap[Equalizer::bandLevels] = std::make_unique<Equalizer>(std::move(eq));
        mBandLevels = bandLevels;
    }
    void setBandCapabilities(std::vector<Equalizer::BandCapability> bandCaps) {
        Equalizer eq;
        eq.set<Equalizer::bandCapability>(bandCaps);
        mTagMap[Equalizer::bandCapability] = std::make_unique<Equalizer>(std::move(eq));
        mBandCapability = bandCaps;
    }

  private:
    Equalizer::VendorExtension mVendorExtension;
    std::vector<Equalizer::BandCapability> mBandCapability;
    std::vector<Equalizer::Preset> mPresets;
    int mPresetIndex;
    std::vector<Equalizer::BandLevel> mBandLevels;

    std::map<Equalizer::Tag, std::unique_ptr<Equalizer>> mTagMap;

    Equalizer::Tag minSetTag = static_cast<Equalizer::Tag>(Equalizer::SetParameterRange::MIN);
    Equalizer::Tag maxSetTag = static_cast<Equalizer::Tag>(Equalizer::SetParameterRange::MAX);
    Equalizer::Tag minGetTag = static_cast<Equalizer::Tag>(Equalizer::GetParameterRange::MIN);
    Equalizer::Tag maxGetTag = static_cast<Equalizer::Tag>(Equalizer::GetParameterRange::MAX);
};

// open/close EqualizerTypeUUID
TEST_P(EqualizerTest, OpenCloseTest) {}

// go over all parameters, expect EX_ILLEGAL_ARGUMENT for parameter out of range
TEST_P(EqualizerTest, SetAndGetAllParametersDefault) {
    SetAndGetEqualizerParameters();
}

// go over all parameters, expect EX_ILLEGAL_ARGUMENT for parameter out of range
TEST_P(EqualizerTest, SetAndGetAllParameters) {
    // TODO: add preset index in and out of supported Capability.BandLevel[] range
    setPreset(1);
    SetAndGetEqualizerParameters();
}

INSTANTIATE_TEST_SUITE_P(EqualizerTest, EqualizerTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
