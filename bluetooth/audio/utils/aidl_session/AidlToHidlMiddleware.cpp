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

#define LOG_TAG "BtAudioNakahara"

#include "AidlToHidlMiddleware.h"

#include <android-base/logging.h>
#include <android/hardware/bluetooth/audio/2.1/types.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "../aidl_session/BluetoothAudioSessionControl.h"
#include "../session/BluetoothAudioSessionControl_2_1.h"
#include "BluetoothAudioSession.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using PcmConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::PcmParameters;
using SampleRate_2_0 = ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ChannelMode_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using BitsPerSample_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using CodecConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using CodecType_2_0 = ::android::hardware::bluetooth::audio::V2_0::CodecType;
using SbcConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SbcParameters;
using AacConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::AacParameters;
using LdacConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::LdacParameters;
using AptxConfig_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::AptxParameters;
using SbcAllocMethod_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SbcAllocMethod;
using SbcBlockLength_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SbcBlockLength;
using SbcChannelMode_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;
using SbcNumSubbands_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SbcNumSubbands;
using AacObjectType_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::AacObjectType;
using AacVarBitRate_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::AacVariableBitRate;
using LdacChannelMode_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::LdacChannelMode;
using LdacQualityIndex_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::LdacQualityIndex;

using PcmConfig_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::PcmParameters;
using SampleRate_2_1 = ::android::hardware::bluetooth::audio::V2_1::SampleRate;
using Lc3CodecConfig_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::Lc3CodecConfiguration;
using Lc3Config_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::Lc3Parameters;
using Lc3FrameDuration_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::Lc3FrameDuration;

using AudioConfig_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::AudioConfiguration;
using SessionControl_2_1 =
    ::android::bluetooth::audio::BluetoothAudioSessionControl_2_1;
using PortStatusCallbacksHidl =
    ::android::bluetooth::audio::PortStatusCallbacks;
using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;
using HidlStatus = ::android::hardware::bluetooth::audio::V2_0::Status;

std::mutex callback_lock;
std::unordered_map<uint16_t, std::shared_ptr<PortStatusCallbacks>>
    callback_table;

const static std::unordered_map<SessionType, SessionType_2_1>
    session_type_to_hidl_map{
        {SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
         SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH},
        {SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::HEARING_AID_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_SOFTWARE_DECODED_DATAPATH},
        {SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH},
        {SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH,
         SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH},
    };

const static std::unordered_map<SampleRate_2_1, int32_t>
    sample_rate_to_aidl_map{
        {SampleRate_2_1::RATE_44100, 44100},
        {SampleRate_2_1::RATE_48000, 48000},
        {SampleRate_2_1::RATE_88200, 88200},
        {SampleRate_2_1::RATE_96000, 96000},
        {SampleRate_2_1::RATE_176400, 176400},
        {SampleRate_2_1::RATE_192000, 192000},
        {SampleRate_2_1::RATE_16000, 16000},
        {SampleRate_2_1::RATE_24000, 24000},
        {SampleRate_2_1::RATE_8000, 8000},
        {SampleRate_2_1::RATE_32000, 32000},
    };

const static std::unordered_map<CodecType_2_0, CodecType>
    codec_type_to_aidl_map{
        {CodecType_2_0::UNKNOWN, CodecType::UNKNOWN},
        {CodecType_2_0::SBC, CodecType::SBC},
        {CodecType_2_0::AAC, CodecType::AAC},
        {CodecType_2_0::APTX, CodecType::APTX},
        {CodecType_2_0::APTX_HD, CodecType::APTX_HD},
        {CodecType_2_0::LDAC, CodecType::LDAC},
    };

const static std::unordered_map<SbcChannelMode_2_0, SbcChannelMode>
    sbc_channel_mode_to_aidl_map{
        {SbcChannelMode_2_0::UNKNOWN, SbcChannelMode::UNKNOWN},
        {SbcChannelMode_2_0::JOINT_STEREO, SbcChannelMode::JOINT_STEREO},
        {SbcChannelMode_2_0::STEREO, SbcChannelMode::STEREO},
        {SbcChannelMode_2_0::DUAL, SbcChannelMode::DUAL},
        {SbcChannelMode_2_0::MONO, SbcChannelMode::MONO},
    };

const static std::unordered_map<SbcBlockLength_2_0, int8_t>
    sbc_block_length_to_aidl_map{
        {SbcBlockLength_2_0::BLOCKS_4, 4},
        {SbcBlockLength_2_0::BLOCKS_8, 8},
        {SbcBlockLength_2_0::BLOCKS_12, 12},
        {SbcBlockLength_2_0::BLOCKS_16, 16},
    };

const static std::unordered_map<SbcNumSubbands_2_0, int8_t>
    sbc_subbands_to_aidl_map{
        {SbcNumSubbands_2_0::SUBBAND_4, 4},
        {SbcNumSubbands_2_0::SUBBAND_8, 8},
    };

const static std::unordered_map<SbcAllocMethod_2_0, SbcAllocMethod>
    sbc_alloc_method_to_aidl_map{
        {SbcAllocMethod_2_0::ALLOC_MD_S, SbcAllocMethod::ALLOC_MD_S},
        {SbcAllocMethod_2_0::ALLOC_MD_L, SbcAllocMethod::ALLOC_MD_L},
    };

const static std::unordered_map<AacObjectType_2_0, AacObjectType>
    aac_object_type_to_aidl_map{
        {AacObjectType_2_0::MPEG2_LC, AacObjectType::MPEG2_LC},
        {AacObjectType_2_0::MPEG4_LC, AacObjectType::MPEG4_LC},
        {AacObjectType_2_0::MPEG4_LTP, AacObjectType::MPEG4_LTP},
        {AacObjectType_2_0::MPEG4_SCALABLE, AacObjectType::MPEG4_SCALABLE},
    };

const static std::unordered_map<LdacChannelMode_2_0, LdacChannelMode>
    ldac_channel_mode_to_aidl_map{
        {LdacChannelMode_2_0::UNKNOWN, LdacChannelMode::UNKNOWN},
        {LdacChannelMode_2_0::STEREO, LdacChannelMode::STEREO},
        {LdacChannelMode_2_0::DUAL, LdacChannelMode::DUAL},
        {LdacChannelMode_2_0::MONO, LdacChannelMode::MONO},
    };

const static std::unordered_map<LdacQualityIndex_2_0, LdacQualityIndex>
    ldac_qindex_to_aidl_map{
        {LdacQualityIndex_2_0::QUALITY_HIGH, LdacQualityIndex::HIGH},
        {LdacQualityIndex_2_0::QUALITY_MID, LdacQualityIndex::MID},
        {LdacQualityIndex_2_0::QUALITY_LOW, LdacQualityIndex::LOW},
        {LdacQualityIndex_2_0::QUALITY_ABR, LdacQualityIndex::ABR},
    };

inline SessionType_2_1 to_hidl_session_type(
    const SessionType& session_type_aidl) {
  auto it = session_type_to_hidl_map.find(session_type_aidl);
  if (it != session_type_to_hidl_map.end()) return it->second;
  return SessionType_2_1::UNKNOWN;
}

inline BluetoothAudioStatus to_aidl_status(const HidlStatus& status) {
  switch (status) {
    case HidlStatus::SUCCESS:
      return BluetoothAudioStatus::SUCCESS;
    case HidlStatus::UNSUPPORTED_CODEC_CONFIGURATION:
      return BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION;
    default:
      return BluetoothAudioStatus::FAILURE;
  }
}

inline int32_t to_aidl_sample_rate(const SampleRate_2_1 sample_rate) {
  auto it = sample_rate_to_aidl_map.find(sample_rate);
  if (it != sample_rate_to_aidl_map.end()) return it->second;
  return 0;
}

inline int8_t to_aidl_bits_per_sample(const BitsPerSample_2_0 bit_per_sample) {
  switch (bit_per_sample) {
    case BitsPerSample_2_0::BITS_16:
      return 16;
    case BitsPerSample_2_0::BITS_24:
      return 24;
    case BitsPerSample_2_0::BITS_32:
      return 32;
    case BitsPerSample_2_0::BITS_UNKNOWN:
      return 0;
  }
}

inline ChannelMode to_aidl_channel_mode(const ChannelMode_2_0 channel_mode) {
  switch (channel_mode) {
    case ChannelMode_2_0::MONO:
      return ChannelMode::MONO;
    case ChannelMode_2_0::STEREO:
      return ChannelMode::STEREO;
    default:
      return ChannelMode::UNKNOWN;
  }
}

inline PcmConfiguration to_aidl_pcm_config(const PcmConfig_2_1& pcm_config) {
  return {
      .sampleRateHz = to_aidl_sample_rate(pcm_config.sampleRate),
      .bitsPerSample = to_aidl_bits_per_sample(pcm_config.bitsPerSample),
      .channelMode = to_aidl_channel_mode(pcm_config.channelMode),
      .dataIntervalUs = static_cast<int32_t>(pcm_config.dataIntervalUs),
  };
}

inline CodecType to_aidl_codec_type(const CodecType_2_0 codec_type) {
  auto it = codec_type_to_aidl_map.find(codec_type);
  if (it != codec_type_to_aidl_map.end()) return it->second;
  return CodecType::UNKNOWN;
}

inline SbcConfiguration to_aidl_sbc_config(const SbcConfig_2_0& sbc_config) {
  SbcConfiguration aidl_sbc_config = {
      .minBitpool = sbc_config.minBitpool,
      .maxBitpool = sbc_config.maxBitpool,
      .sampleRateHz = to_aidl_sample_rate(
          static_cast<SampleRate_2_1>(sbc_config.sampleRate)),
      .bitsPerSample = to_aidl_bits_per_sample(sbc_config.bitsPerSample),
  };

  if (sbc_channel_mode_to_aidl_map.find(sbc_config.channelMode) !=
      sbc_channel_mode_to_aidl_map.end()) {
    aidl_sbc_config.channelMode =
        sbc_channel_mode_to_aidl_map.at(sbc_config.channelMode);
  }
  if (sbc_block_length_to_aidl_map.find(sbc_config.blockLength) !=
      sbc_block_length_to_aidl_map.end()) {
    aidl_sbc_config.blockLength =
        sbc_block_length_to_aidl_map.at(sbc_config.blockLength);
  }
  if (sbc_subbands_to_aidl_map.find(sbc_config.numSubbands) !=
      sbc_subbands_to_aidl_map.end()) {
    aidl_sbc_config.numSubbands =
        sbc_subbands_to_aidl_map.at(sbc_config.numSubbands);
  }
  if (sbc_alloc_method_to_aidl_map.find(sbc_config.allocMethod) !=
      sbc_alloc_method_to_aidl_map.end()) {
    aidl_sbc_config.allocMethod =
        sbc_alloc_method_to_aidl_map.at(sbc_config.allocMethod);
  }
  return aidl_sbc_config;
}

inline AacConfiguration to_aidl_aac_config(const AacConfig_2_0& aac_config) {
  AacConfiguration aidl_aac_config = {
      .sampleRateHz = to_aidl_sample_rate(
          static_cast<SampleRate_2_1>(aac_config.sampleRate)),
      .bitsPerSample = to_aidl_bits_per_sample(aac_config.bitsPerSample),
      .channelMode = to_aidl_channel_mode(aac_config.channelMode),
  };

  if (aac_object_type_to_aidl_map.find(aac_config.objectType) !=
      aac_object_type_to_aidl_map.end()) {
    aidl_aac_config.objectType =
        aac_object_type_to_aidl_map.at(aac_config.objectType);
  }
  aidl_aac_config.variableBitRateEnabled =
      (aac_config.variableBitRateEnabled == AacVarBitRate_2_0::ENABLED);
  return aidl_aac_config;
}

inline LdacConfiguration to_aidl_ldac_config(
    const LdacConfig_2_0& ldac_config) {
  LdacConfiguration aidl_ldac_config = {
      .sampleRateHz = to_aidl_sample_rate(
          static_cast<SampleRate_2_1>(ldac_config.sampleRate)),
      .bitsPerSample = to_aidl_bits_per_sample(ldac_config.bitsPerSample),
  };

  if (ldac_channel_mode_to_aidl_map.find(ldac_config.channelMode) !=
      ldac_channel_mode_to_aidl_map.end()) {
    aidl_ldac_config.channelMode =
        ldac_channel_mode_to_aidl_map.at(ldac_config.channelMode);
  }
  if (ldac_qindex_to_aidl_map.find(ldac_config.qualityIndex) !=
      ldac_qindex_to_aidl_map.end()) {
    aidl_ldac_config.qualityIndex =
        ldac_qindex_to_aidl_map.at(ldac_config.qualityIndex);
  }
  return aidl_ldac_config;
}

inline AptxConfiguration to_aidl_aptx_config(
    const AptxConfig_2_0& aptx_config) {
  return {
      .sampleRateHz = to_aidl_sample_rate(
          static_cast<SampleRate_2_1>(aptx_config.sampleRate)),
      .bitsPerSample = to_aidl_bits_per_sample(aptx_config.bitsPerSample),
      .channelMode = to_aidl_channel_mode(aptx_config.channelMode),
  };
}

inline CodecConfiguration to_aidl_codec_config(
    const CodecConfig_2_0& codec_config) {
  CodecConfiguration aidl_codec_config = {
      .codecType = to_aidl_codec_type(codec_config.codecType),
      .encodedAudioBitrate =
          static_cast<int32_t>(codec_config.encodedAudioBitrate),
      .peerMtu = static_cast<int32_t>(codec_config.peerMtu),
      .isScmstEnabled = codec_config.isScmstEnabled,
  };

  switch (codec_config.config.getDiscriminator()) {
    case CodecConfig_2_0::CodecSpecific::hidl_discriminator::sbcConfig:
      aidl_codec_config.config
          .set<CodecConfiguration::CodecSpecific::sbcConfig>(
              to_aidl_sbc_config(codec_config.config.sbcConfig()));
      break;
    case CodecConfig_2_0::CodecSpecific::hidl_discriminator::aacConfig:
      aidl_codec_config.config
          .set<CodecConfiguration::CodecSpecific::aacConfig>(
              to_aidl_aac_config(codec_config.config.aacConfig()));
      break;
    case CodecConfig_2_0::CodecSpecific::hidl_discriminator::ldacConfig:
      aidl_codec_config.config
          .set<CodecConfiguration::CodecSpecific::ldacConfig>(
              to_aidl_ldac_config(codec_config.config.ldacConfig()));
      break;
    case CodecConfig_2_0::CodecSpecific::hidl_discriminator::aptxConfig:
      aidl_codec_config.config
          .set<CodecConfiguration::CodecSpecific::aptxConfig>(
              to_aidl_aptx_config(codec_config.config.aptxConfig()));
      break;
    default:
      break;
  }
  return aidl_codec_config;
}

inline AudioConfiguration to_aidl_audio_config(
    const AudioConfig_2_1& audio_config) {
  switch (audio_config.getDiscriminator()) {
    case AudioConfig_2_1::hidl_discriminator::pcmConfig:
      return AudioConfiguration(to_aidl_pcm_config(audio_config.pcmConfig()));
    case AudioConfig_2_1::hidl_discriminator::codecConfig:
      return AudioConfiguration(
          to_aidl_codec_config(audio_config.codecConfig()));
    case AudioConfig_2_1::hidl_discriminator::leAudioCodecConfig:
      LOG(ERROR) << __func__
                 << " LE Audio offloading should not be supported in HIDL!";
      break;
  }
  return {};
}

/***
 *
 * Implementation
 *
 ***/

bool AidlToHidlMiddleware::IsSessionReady(const SessionType& session_type) {
  return SessionControl_2_1::IsSessionReady(to_hidl_session_type(session_type));
}

static void control_status_callback(uint16_t cookie, bool start_resp,
                                    const HidlStatus& status) {
  auto it = callback_table.find(cookie);
  if (it == callback_table.end()) {
    return;
  }
  auto cbacks = it->second;
  if (cbacks->control_result_cb_)
    cbacks->control_result_cb_(cookie, start_resp, to_aidl_status(status));
}

static void session_changed_cb(uint16_t cookie) {
  auto it = callback_table.find(cookie);
  if (it == callback_table.end()) {
    return;
  }
  auto cbacks = it->second;
  if (cbacks->session_changed_cb_) cbacks->session_changed_cb_(cookie);
}

PortStatusCallbacksHidl port_status_callbacks = {
    .control_result_cb_ = control_status_callback,
    .session_changed_cb_ = session_changed_cb,
};

uint16_t AidlToHidlMiddleware::RegisterStatusCback(
    const SessionType& session_type, const PortStatusCallbacks& cbacks) {
  LOG(INFO) << __func__ << ": " << toString(session_type);

  auto cookie = SessionControl_2_1::RegisterControlResultCback(
      to_hidl_session_type(session_type), port_status_callbacks);
  {
    std::lock_guard<std::mutex> guard(callback_lock);
    callback_table[cookie] = std::make_shared<PortStatusCallbacks>(cbacks);
  }
  return cookie;
}

void AidlToHidlMiddleware::UnregisterStatusCback(
    const SessionType& session_type, uint16_t cookie) {
  LOG(INFO) << __func__ << ": " << toString(session_type);
  SessionControl_2_1::UnregisterControlResultCback(
      to_hidl_session_type(session_type), cookie);

  auto it = callback_table.find(cookie);
  if (it != callback_table.end()) {
    std::lock_guard<std::mutex> guard(callback_lock);
    callback_table.erase(it);
  }
}

const AudioConfiguration AidlToHidlMiddleware::GetAudioConfig(
    const SessionType& session_type) {
  return to_aidl_audio_config(
      SessionControl_2_1::GetAudioConfig(to_hidl_session_type(session_type)));
}

bool AidlToHidlMiddleware::StartStream(const SessionType& session_type) {
  return SessionControl_2_1::StartStream(to_hidl_session_type(session_type));
}

void AidlToHidlMiddleware::StopStream(const SessionType& session_type) {
  return SessionControl_2_1::StopStream(to_hidl_session_type(session_type));
}

bool AidlToHidlMiddleware::SuspendStream(const SessionType& session_type) {
  return SessionControl_2_1::SuspendStream(to_hidl_session_type(session_type));
}

bool AidlToHidlMiddleware::GetPresentationPosition(
    const SessionType& session_type,
    PresentationPosition& presentation_position) {
  uint64_t remote_delay_report_ns;
  uint64_t total_bytes_readed;
  timespec data_position;
  auto ret_val = SessionControl_2_1::GetPresentationPosition(
      to_hidl_session_type(session_type), &remote_delay_report_ns,
      &total_bytes_readed, &data_position);

  presentation_position = {
      .remoteDeviceAudioDelayNanos =
          static_cast<int64_t>(remote_delay_report_ns),
      .transmittedOctets = static_cast<int64_t>(total_bytes_readed),
      .transmittedOctetsTimestamp =
          {
              .tvSec = static_cast<int64_t>(data_position.tv_sec),
              .tvNSec = static_cast<int64_t>(data_position.tv_nsec),
          },
  };
  return ret_val;
}

void AidlToHidlMiddleware::UpdateSourceMetadata(
    const SessionType& session_type,
    const struct source_metadata& source_metadata) {
  return SessionControl_2_1::UpdateTracksMetadata(
      to_hidl_session_type(session_type), &source_metadata);
}

void AidlToHidlMiddleware::UpdateSinkMetadata(const SessionType&,
                                              const struct sink_metadata&) {
  LOG(ERROR) << __func__ << " not supported in HIDL";
}

size_t AidlToHidlMiddleware::OutWritePcmData(const SessionType& session_type,
                                             const void* buffer, size_t bytes) {
  return SessionControl_2_1::OutWritePcmData(to_hidl_session_type(session_type),
                                             buffer, bytes);
}

size_t AidlToHidlMiddleware::InReadPcmData(const SessionType& session_type,
                                           void* buffer, size_t bytes) {
  return SessionControl_2_1::InReadPcmData(to_hidl_session_type(session_type),
                                           buffer, bytes);
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl