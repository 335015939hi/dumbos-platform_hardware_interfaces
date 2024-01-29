/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "BluetoothHfpCodecsProvider.h"

#include "aidl_android_hardware_bluetooth_audio_hfp_setting_enums.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace hfp {

static const char* kHfpCodecCapabilitiesFile =
    "/vendor/etc/aidl/hfp/hfp_codec_capabilities.xml";

std::optional<setting::HfpOffloadSetting>
BluetoothHfpCodecsProvider::ParseFromHfpOffloadSettingFile() {
  auto hfp_offload_setting =
      setting::readHfpOffloadSetting(kHfpCodecCapabilitiesFile);
  if (!hfp_offload_setting.has_value()) {
    LOG(ERROR) << __func__ << ": Failed to read " << kHfpCodecCapabilitiesFile;
  }
  return hfp_offload_setting;
}

std::vector<CodecInfo> BluetoothHfpCodecsProvider::GetHfpAudioCodecInfo(
    const std::optional<setting::HfpOffloadSetting>& hfp_offload_setting) {
  std::vector<CodecInfo> result;
  if (!hfp_offload_setting.has_value()) return result;
  for (auto& cf : hfp_offload_setting.value().getConfiguration()) {
    CodecInfo codec_info;

    switch (cf.getCodec()) {
      case setting::CodecType::LC3:
        codec_info.id = CodecId::Core::LC3;
        break;
      case setting::CodecType::MSBC:
        codec_info.id = CodecId::Core::MSBC;
        break;
      case setting::CodecType::CVSD:
        codec_info.id = CodecId::Core::CVSD;
        break;
      default:
        LOG(WARNING) << __func__ << ": Unknown codec from " << cf.getName();
        codec_info.id = CodecId::Vendor();
        break;
    }
    codec_info.name = cf.getName();

    codec_info.transport =
        CodecInfo::Transport::make<CodecInfo::Transport::Tag::hfp>();

    auto& transport =
        codec_info.transport.get<CodecInfo::Transport::Tag::hfp>();
    transport.useControllerCodec = cf.getUseControllerCodec();
    transport.inputDataPath = cf.getInputDataPath();
    transport.outputDataPath = cf.getOutputDataPath();

    result.push_back(codec_info);
  }
  LOG(INFO) << __func__ << ": Has " << result.size() << " codec info";
  return result;
}

}  // namespace hfp
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
