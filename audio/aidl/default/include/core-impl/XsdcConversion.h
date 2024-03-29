#include <string>
#include <unordered_map>
#include <unordered_set>

#include <aidl/android/media/audio/common/AudioHalCapCriterion.h>
#include <aidl/android/media/audio/common/AudioHalCapCriterionType.h>
#include <aidl/android/media/audio/common/AudioHalVolumeCurve.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <android_audio_policy_configuration.h>
#include <android_audio_policy_configuration_enums.h>
#include <android_audio_policy_engine_configuration.h>
#include <media/AidlConversionUtil.h>

#include "core-impl/Module.h"

namespace aidl::android::hardware::audio::core::internal {

namespace engineconfiguration = ::android::audio::policy::engine::configuration;
namespace aidlaudiocommon = ::aidl::android::media::audio::common;

ConversionResult<aidlaudiocommon::AudioHalCapCriterion>
convertCapCriterionToAidl(const engineconfiguration::CriterionType& xsdcCriterion);
ConversionResult<aidlaudiocommon::AudioHalCapCriterionType>
convertCapCriterionTypeToAidl(const engineconfiguration::CriterionTypeType& xsdcCriterionType);
ConversionResult<aidlaudiocommon::AudioHalVolumeCurve::CurvePoint>
convertCurvePointToAidl(const std::string& xsdcCurvePoint);
ConversionResult<std::unique_ptr<Module::Configuration>> convertModuleConfigToAidl(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
ConversionResult<aidlaudiocommon::AudioUsage>
        convertAudioUsageToAidl(const engineconfiguration::UsageEnumType& xsdcUsage);
ConversionResult<aidlaudiocommon::AudioContentType>
        convertAudioContentTypeToAidl(const engineconfiguration::ContentType& xsdcContentType);
ConversionResult<aidlaudiocommon::AudioSource>
        convertAudioSourceToAidl(const engineconfiguration::SourceEnumType& xsdcSourceType);
ConversionResult<aidlaudiocommon::AudioStreamType>
        convertAudioStreamTypeToAidl(const engineconfiguration::Stream& xsdStreamType);
ConversionResult<int32_t> convertAudioFlagsToAidl(
        const std::vector<engineconfiguration::FlagType>& xsdcFlagTypeVec);
}  // namespace aidl::android::hardware::audio::core::internal
