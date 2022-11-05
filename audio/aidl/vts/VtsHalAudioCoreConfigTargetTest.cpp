#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define LOG_TAG "VtsHalAudioCore.Config"

#include <Utils.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/core/IConfig.h>

#include "AudioHalBinderServiceUtil.h"
#include "TestUtils.h"

using namespace android;
using aidl::android::hardware::audio::core::IConfig;
using aidl::android::media::audio::common::AudioAttributes;
using aidl::android::media::audio::common::AudioFlag;
using aidl::android::media::audio::common::AudioHalAttributesGroup;
using aidl::android::media::audio::common::AudioHalCapCriterion;
using aidl::android::media::audio::common::AudioHalCapCriterionType;
using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalProductStrategy;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioProductStrategyType;
using aidl::android::media::audio::common::AudioStreamType;

static const int KVolumeIndexDeferredToAudioService = -1;

template <typename T>
bool isEnumValueReservedForSystemUse(const T& enumVal) {
    return toString(enumVal).find("SYS_RESERVED_") != std::string::npos;
}

class AudioCoreConfig : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(ConnectToService()); }
    void ConnectToService() {
        mConfig = IConfig::fromBinder(mBinderUtil.connectToService(GetParam()));
        ASSERT_NE(mConfig, nullptr);
    }
    void RestartService() {
        ASSERT_NE(mConfig, nullptr);
        mConfig = IConfig::fromBinder(mBinderUtil.restartService());
        ASSERT_NE(mConfig, nullptr);
    }
    void SetUpEngineConfig() {
        if (mEngineConfig == nullptr) {
            AudioHalEngineConfig engConfig;
            ASSERT_IS_OK(mConfig->getEngineConfig(&engConfig));
            mEngineConfig = std::make_unique<AudioHalEngineConfig>(engConfig);
        }
    }
    const std::unordered_set<int>& GetSupportedAudioProductStrategyTypes() {
        static const std::unordered_set<int> supportedAudioProductStrategyTypes = []() {
            std::unordered_set<int> supportedStrategyTypes;
            for (const auto& audioProductStrategyType :
                 ndk::enum_range<AudioProductStrategyType>()) {
                if (!isEnumValueReservedForSystemUse(audioProductStrategyType)) {
                    supportedStrategyTypes.insert(static_cast<int>(audioProductStrategyType));
                }
            }
            return supportedStrategyTypes;
        }();
        return supportedAudioProductStrategyTypes;
    }
    const int& GetSupportedAudioFlagsMask() {
        static const int supportedAudioFlagsMask = []() {
            int mask = 0;
            for (const auto& audioFlag : ndk::enum_range<AudioFlag>()) {
                if (!isEnumValueReservedForSystemUse(audioFlag)) {
                    mask |= static_cast<int>(audioFlag);
                }
            }
            return mask;
        }();
        return supportedAudioFlagsMask;
    }
    /**
     * Verify streamType is not INVALID if we are using the default engine
     */
    void ValidateAudioStreamType(const AudioStreamType& streamType) {
        EXPECT_FALSE(isEnumValueReservedForSystemUse(streamType));
        if (!mEngineConfig->capSpecificConfig) {
            EXPECT_NE(streamType, AudioStreamType::INVALID);
        }
    }
    /**
     * Verify contained enum types are valid
     */
    void ValidateAudioAttributes(const AudioAttributes& attributes) {
        EXPECT_FALSE(isEnumValueReservedForSystemUse(attributes.contentType));
        EXPECT_FALSE(isEnumValueReservedForSystemUse(attributes.usage));
        EXPECT_FALSE(isEnumValueReservedForSystemUse(attributes.source));
        EXPECT_EQ(attributes.flags & ~GetSupportedAudioFlagsMask(), 0);
    }
    /**
     * Verify volumeGroupName corresponds to an AudioHalVolumeGroup
     * Validate contained types
     */
    void ValidateAudioHalAttributesGroup(
            const AudioHalAttributesGroup& attributesGroup,
            std::unordered_set<std::string>& volumeGroupNameSet,
            std::unordered_set<std::string>& volumeGroupsUsedInStrategies) {
        ValidateAudioStreamType(attributesGroup.streamType);
        bool isVolumeGroupNameValid = volumeGroupNameSet.count(attributesGroup.volumeGroupName);
        EXPECT_TRUE(isVolumeGroupNameValid);
        if (isVolumeGroupNameValid) {
            volumeGroupsUsedInStrategies.insert(attributesGroup.volumeGroupName);
        }
        for (const AudioAttributes& attr : attributesGroup.attributes) {
            ValidateAudioAttributes(attr);
        }
    }
    /**
     * Default engine: verify productStrategy.id is valid AudioProductStrategyType
     * CAP engine: verify productStrategy.id is either valid AudioProductStrategyType OR >=
     * VENDOR_STRATEGY_ID_START Validate contained types
     */
    void ValidateAudioHalProductStrategy(
            const AudioHalProductStrategy& strategy,
            std::unordered_set<std::string>& volumeGroupNameSet,
            std::unordered_set<std::string>& volumeGroupsUsedInStrategies) {
        if (!mEngineConfig->capSpecificConfig ||
            (strategy.id < AudioHalProductStrategy::VENDOR_STRATEGY_ID_START)) {
            EXPECT_NE(GetSupportedAudioProductStrategyTypes().find(strategy.id),
                      GetSupportedAudioProductStrategyTypes().end());
        }
        for (const AudioHalAttributesGroup& attributesGroup : strategy.attributesGroups) {
            ValidateAudioHalAttributesGroup(attributesGroup, volumeGroupNameSet,
                                            volumeGroupsUsedInStrategies);
        }
    }
    /**
     * Verify curve point index is in [0,100]
     */
    void ValidateAudioHalVolumeCurve(const AudioHalVolumeCurve& volumeCurve) {
        for (const AudioHalVolumeCurve::CurvePoint& curvePoint : volumeCurve.curvePoints) {
            EXPECT_TRUE(curvePoint.index >= 0);
            EXPECT_TRUE(curvePoint.index <= 100);
        }
    }
    /**
     * Verify minIndex, maxIndex are non-negative
     * Verify minIndex <= maxIndex
     * Verify no two volume curves use the same device category
     * Validate contained types
     */
    void ValidateAudioHalVolumeGroup(const AudioHalVolumeGroup& volumeGroup) {
        /**
         * Legacy volume curves in audio_policy_configuration.xsd don't use
         * minIndex or maxIndex. Use of audio_policy_configuration.xml still
         * allows, and in some cases, relies on, AudioService to provide the min
         * and max indices for a volumeGroup. From the VTS perspective, there is
         * no way to differentiate between use of audio_policy_configuration.xml
         * or audio_policy_engine_configuration.xml, as either one can be used
         * for the default audio policy engine.
         */
        if (volumeGroup.minIndex != KVolumeIndexDeferredToAudioService ||
            volumeGroup.maxIndex != KVolumeIndexDeferredToAudioService) {
            EXPECT_TRUE(volumeGroup.minIndex >= 0);
            EXPECT_TRUE(volumeGroup.maxIndex >= 0);
        }
        EXPECT_TRUE(volumeGroup.minIndex <= volumeGroup.maxIndex);
        std::unordered_set<AudioHalVolumeCurve::DeviceCategory> deviceCategorySet;
        for (const AudioHalVolumeCurve& volumeCurve : volumeGroup.volumeCurves) {
            EXPECT_TRUE(deviceCategorySet.insert(volumeCurve.deviceCategory).second);
            ValidateAudioHalVolumeCurve(volumeCurve);
        }
    }
    /**
     * Verify defaultLiteralValue is empty for inclusive criterion
     */
    void ValidateAudioHalCapCriterion(const AudioHalCapCriterion& criterion,
                                      const AudioHalCapCriterionType& criterionType) {
        if (criterionType.isInclusive) {
            EXPECT_TRUE(criterion.defaultLiteralValue.empty());
        }
    }
    /**
     * Verify each criterionType has a unique name
     * Verify each criterion has a unique name
     * Verify each criterion maps to a criterionType
     * Verify each criterionType is used in a criterion
     * Validate contained types
     */
    void ValidateCapSpecificConfig(const AudioHalEngineConfig::CapSpecificConfig capCfg) {
        std::unordered_map<std::string, AudioHalCapCriterionType> criterionTypeMap;
        for (const AudioHalCapCriterionType& criterionType : capCfg.criterionTypes) {
            EXPECT_TRUE(criterionTypeMap.insert({criterionType.name, criterionType}).second);
        }
        std::unordered_set<std::string> criterionNameSet;
        for (const AudioHalCapCriterion& criterion : capCfg.criteria) {
            EXPECT_TRUE(criterionNameSet.insert(criterion.name).second);
            EXPECT_EQ(criterionTypeMap.count(criterion.criterionTypeName), 1UL);
            ValidateAudioHalCapCriterion(criterion,
                                         criterionTypeMap.at(criterion.criterionTypeName));
        }
        EXPECT_EQ(criterionTypeMap.size(), criterionNameSet.size());
    }
    /**
     * Verify VolumeGroups are non-empty
     * Verify defaultProductStrategyId is matches one of the provided productStrategies. Otherwise,
     * must be left uninitialized. Verify each volumeGroup has a unique name Verify each
     * productStrategy has a unique id Verify each volumeGroup is used in a product strategy
     * Validate contained types
     */
    void ValidateAudioHalEngineConfig() {
        EXPECT_NE(mEngineConfig->volumeGroups.size(), 0UL);
        std::unordered_set<std::string> volumeGroupNameSet;
        for (const AudioHalVolumeGroup& volumeGroup : mEngineConfig->volumeGroups) {
            EXPECT_TRUE(volumeGroupNameSet.insert(volumeGroup.name).second);
            ValidateAudioHalVolumeGroup(volumeGroup);
        }
        if (!mEngineConfig->productStrategies.empty()) {
            std::unordered_set<int> productStrategyIdSet;
            std::unordered_set<std::string> volumeGroupsUsedInStrategies;
            for (const AudioHalProductStrategy& strategy : mEngineConfig->productStrategies) {
                EXPECT_TRUE(productStrategyIdSet.insert(strategy.id).second);
                ValidateAudioHalProductStrategy(strategy, volumeGroupNameSet,
                                                volumeGroupsUsedInStrategies);
            }
            EXPECT_TRUE(productStrategyIdSet.count(mEngineConfig->defaultProductStrategyId))
                    << "defaultProductStrategyId doesn't match any of the provided "
                       "productStrategies";
            EXPECT_EQ(volumeGroupNameSet.size(), volumeGroupsUsedInStrategies.size());
        } else {
            EXPECT_EQ(mEngineConfig->defaultProductStrategyId,
                      static_cast<int>(AudioProductStrategyType::SYS_RESERVED_NONE))
                    << "defaultProductStrategyId defined, but no productStrategies were provided";
        }
        if (mEngineConfig->capSpecificConfig) {
            ValidateCapSpecificConfig(mEngineConfig->capSpecificConfig.value());
        }
    }

  private:
    std::shared_ptr<IConfig> mConfig;
    std::unique_ptr<AudioHalEngineConfig> mEngineConfig;
    AudioHalBinderServiceUtil mBinderUtil;
};
TEST_P(AudioCoreConfig, Published) {
    // SetUp must complete with no failures.
}
TEST_P(AudioCoreConfig, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(RestartService());
}
TEST_P(AudioCoreConfig, GetEngineConfigIsValid) {
    ASSERT_NO_FATAL_FAILURE(SetUpEngineConfig());
    ValidateAudioHalEngineConfig();
}

INSTANTIATE_TEST_SUITE_P(AudioCoreConfigTest, AudioCoreConfig,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IConfig::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioCoreConfig);
