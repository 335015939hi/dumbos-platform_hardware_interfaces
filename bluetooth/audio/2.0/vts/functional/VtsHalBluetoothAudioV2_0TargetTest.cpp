/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_TAG "bluetooth_audio_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvider.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvidersFactory.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <utils/Log.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::AacObjectType;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecCapability;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioProvider;
using ::android::hardware::bluetooth::audio::V2_0::
    IBluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::LdacChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;
using CodecConfigSpecific = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::EncodedDataConfiguration::CodecSpecific;

namespace {
constexpr SessionType session_types[4] = {
    SessionType::UNKNOWN, SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
    SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH,
    SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH};

constexpr SampleRate a2dp_sample_rates[5] = {
    SampleRate::RATE_UNKNOWN, SampleRate::RATE_44100, SampleRate::RATE_48000,
    SampleRate::RATE_88200, SampleRate::RATE_96000};
constexpr BitsPerSample a2dp_bits_per_samples[4] = {
    BitsPerSample::BITS_UNKNOWN, BitsPerSample::BITS_16, BitsPerSample::BITS_24,
    BitsPerSample::BITS_32};
constexpr ChannelMode a2dp_channel_modes[3] = {
    ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
constexpr CodecType a2dp_codec_types[6] = {CodecType::UNKNOWN, CodecType::SBC,
                                           CodecType::AAC,     CodecType::APTX,
                                           CodecType::APTX_HD, CodecType::LDAC};

constexpr SampleRate hearing_aid_sample_rates[3] = {
    SampleRate::RATE_UNKNOWN, SampleRate::RATE_16000, SampleRate::RATE_24000};
constexpr BitsPerSample hearing_aid_bits_per_samples[4] = {
    BitsPerSample::BITS_UNKNOWN, BitsPerSample::BITS_16};
constexpr ChannelMode hearing_aid_channel_modes[3] = {
    ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};

template <typename T>
std::vector<T> ExtractValuesFromBitmask(T bitmasks, uint32_t bitfield,
                                        bool supported) {
  std::vector<T> retval;
  if (!supported) {
    retval.push_back(static_cast<T>(bitfield));
  }
  uint32_t test_bit = 0x00000001;
  while (test_bit <= static_cast<uint32_t>(bitmasks) && test_bit <= bitfield) {
    if ((bitfield & test_bit)) {
      if ((!(bitmasks & test_bit) && !supported) ||
          ((bitmasks & test_bit) && supported)) {
        retval.push_back(static_cast<T>(test_bit));
      }
    }
    test_bit <<= 1;
    if (test_bit == 0x80000000) {
      break;
    }
  }
  return retval;
}
}  // namespace

// Test environment for Bluetooth Audio HAL.
class BluetoothAudioHidlEnvironment
    : public ::testing::VtsHalHidlTargetTestEnvBase {
 public:
  // get the test environment singleton
  static BluetoothAudioHidlEnvironment* Instance() {
    static BluetoothAudioHidlEnvironment* instance =
        new BluetoothAudioHidlEnvironment;
    return instance;
  }

  virtual void registerTestServices() override {
    registerTestService<IBluetoothAudioProvidersFactory>();
  }

 private:
  BluetoothAudioHidlEnvironment() {}
};

// The base test class for Bluetooth Audio HAL.
class BluetoothAudioProvidersFactoryHidlTest
    : public ::testing::VtsHalHidlTargetTestBase {
 public:
  virtual void SetUp() override {
    providers_factory_ = ::testing::VtsHalHidlTargetTestBase::getService<
        IBluetoothAudioProvidersFactory>(
        BluetoothAudioHidlEnvironment::Instance()
            ->getServiceName<IBluetoothAudioProvidersFactory>());
    ASSERT_NE(providers_factory_, nullptr);
  }

  virtual void TearDown() override { providers_factory_ = nullptr; }

  // A simple test implementation of IBluetoothAudioPort.
  class BluetoothAudioPort : public ::testing::VtsHalHidlTargetCallbackBase<
                                 BluetoothAudioProvidersFactoryHidlTest>,
                             public IBluetoothAudioPort {
    BluetoothAudioProvidersFactoryHidlTest& parent_;

   public:
    BluetoothAudioPort(BluetoothAudioProvidersFactoryHidlTest& parent)
        : parent_(parent) {}
    virtual ~BluetoothAudioPort() = default;

    Return<void> startStream() override {
      parent_.audio_provider_->streamStarted(BluetoothAudioStatus::SUCCESS);
      return Void();
    }

    Return<void> suspendStream() override {
      parent_.audio_provider_->streamSuspended(BluetoothAudioStatus::SUCCESS);
      return Void();
    }

    Return<void> stopStream() override { return Void(); }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
      _hidl_cb(BluetoothAudioStatus::SUCCESS, 0, 0, {.tvSec = 0, .tvNSec = 0});
      return Void();
    }

    Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) {
      return Void();
    }
  };

  void OpenProviderHelper(const SessionType& session_type, bool is_mandatory) {
    BluetoothAudioStatus cb_status;
    auto hidl_cb = [&cb_status, &local_provider = this->audio_provider_](
                       BluetoothAudioStatus status,
                       const sp<IBluetoothAudioProvider>& provider) {
      cb_status = status;
      local_provider = provider;
    };
    auto hidl_retval = providers_factory_->openProvider(session_type, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    if (is_mandatory && session_type != SessionType::UNKNOWN) {
      ASSERT_EQ(cb_status, BluetoothAudioStatus::SUCCESS);
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = new BluetoothAudioPort(*this);
    } else if (session_type != SessionType::UNKNOWN &&
               cb_status == BluetoothAudioStatus::SUCCESS) {
      // Optional session_type seems to be supported by DUT
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = new BluetoothAudioPort(*this);
    } else {
      // DUT does not support this session_type or session_type is unknown
      ASSERT_EQ(cb_status, BluetoothAudioStatus::FAILURE);
      ASSERT_EQ(audio_provider_, nullptr);
    }
  }

  sp<IBluetoothAudioProvidersFactory> providers_factory_;

  // audio_provider_ is for the Bluetooth stack to report session started/ended
  // and handled audio stream started / suspended
  sp<IBluetoothAudioProvider> audio_provider_;

  // audio_port_ is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth stack
  sp<IBluetoothAudioPort> audio_port_;
};

TEST_F(BluetoothAudioProvidersFactoryHidlTest, GetProvidersFactoryService) {}

TEST_F(BluetoothAudioProvidersFactoryHidlTest,
       getOffloadCodecCapabilitiesBySession) {
  BluetoothAudioStatus cb_status;
  hidl_vec<CodecCapability> codec_capabilities;
  auto hidl_cb = [&cb_status, &codec_capabilities](
                     BluetoothAudioStatus status,
                     const hidl_vec<CodecCapability>& codecCapabilities) {
    cb_status = status;
    codec_capabilities.resize(codecCapabilities.size());
    for (int i = 0; i < codecCapabilities.size(); ++i) {
      codec_capabilities[i] = codecCapabilities[i];
    }
  };
  for (auto session_type : session_types) {
    auto hidl_retval =
        providers_factory_->getOffloadCodecCapabilities(session_type, hidl_cb);
    ASSERT_TRUE(hidl_retval.isOk());
    if (cb_status != BluetoothAudioStatus::SUCCESS) {
      EXPECT_EQ(cb_status, BluetoothAudioStatus::FAILURE);
      EXPECT_EQ(codec_capabilities.size(), 0);
      continue;
    }
    ASSERT_EQ(session_type, SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH);
    ASSERT_NE(codec_capabilities.size(), 0);
    for (auto codec_capability : codec_capabilities) {
      ASSERT_NE(codec_capability.codecType, CodecType::UNKNOWN);
      switch (codec_capability.codecType) {
        case CodecType::SBC:
          codec_capability.codecSpecific.sbcDataCapability();
          break;
        case CodecType::AAC:
          codec_capability.codecSpecific.aacDataCapability();
          break;
        case CodecType::LDAC:
          codec_capability.codecSpecific.ldacDataCapability();
          break;
        default:
          break;
      }
    }
  }
}

TEST_F(BluetoothAudioProvidersFactoryHidlTest, OpenProviderBySession) {
  for (auto session_type : session_types) {
    if (session_type == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
      OpenProviderHelper(session_type, false);
    } else {
      OpenProviderHelper(session_type, true);
    }
  }
}

/**
 * openProvider A2DP_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderA2dpSoftwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    OpenProviderHelper(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH, true);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }
};

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest, OpenA2dpSoftwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSessionWithPossiblePcmConfig) {
  bool is_codec_config_valid;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    if (is_codec_config_valid) {
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_TRUE(dataMQ.isHandleValid());
      tempDataMQ.reset(new DataMQ(dataMQ));
    } else {
      EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
      EXPECT_FALSE(dataMQ.isHandleValid());
    }
  };
  CodecConfiguration codec_config;
  for (auto sample_rate : a2dp_sample_rates) {
    codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : a2dp_bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : a2dp_channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          is_codec_config_valid = true;
        } else {
          is_codec_config_valid = false;
        }
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk());
        if (is_codec_config_valid) {
          EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

/**
 * openProvider A2DP_HARDWARE_OFFLOAD_DATAPATH
 */
class BluetoothAudioProviderA2dpHardwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    OpenProviderHelper(SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH, false);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }

  void GetOffloadCodecCapabilitiesHelper(const CodecType& codec_type) {
    codec_capability_ = {};
    auto hidl_cb = [codec_type, &codec_capability_cb = this->codec_capability_](
                       BluetoothAudioStatus status,
                       const hidl_vec<CodecCapability>& codecCapabilities) {
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_NE(codecCapabilities.size(), 0);
      for (auto codec_capability : codecCapabilities) {
        if (codec_capability.codecType != codec_type) continue;
        codec_capability_cb = codec_capability;
      }
    };
    auto hidl_retval = providers_factory_->getOffloadCodecCapabilities(
        SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH, hidl_cb);
    ASSERT_TRUE(hidl_retval.isOk());
  }

  std::vector<CodecConfiguration::PcmDataConfiguration>
  GetPcmConfigurationSupportedList() {
    std::vector<SampleRate> sample_rates = ExtractValuesFromBitmask<SampleRate>(
        codec_capability_.pcmDataCapability.sampleRateBitmask, 0xff, true);
    std::vector<BitsPerSample> bits_per_samples =
        ExtractValuesFromBitmask<BitsPerSample>(
            codec_capability_.pcmDataCapability.bitsPerSampleBitmask, 0x7,
            true);
    std::vector<ChannelMode> channel_modes =
        ExtractValuesFromBitmask<ChannelMode>(
            codec_capability_.pcmDataCapability.channelModeBitmask, 0x3, true);
    std::vector<CodecConfiguration::PcmDataConfiguration> pcm_configs;
    for (auto sample_rate : sample_rates) {
      for (auto bits_per_sample : bits_per_samples) {
        for (auto channel_mode : channel_modes) {
          pcm_configs.push_back({.sampleRate = sample_rate,
                                 .bitsPerSample = bits_per_sample,
                                 .channelMode = channel_mode});
        }
      }
    }
    return pcm_configs;
  }

  std::vector<CodecConfigSpecific> GetSbcCodecSpecificSupportedList() {
    CodecCapability::CodecSpecific::SbcDataCapability sbc_capability =
        codec_capability_.codecSpecific.sbcDataCapability();
    std::vector<CodecConfigSpecific> sbc_specifics;
    if (sbc_capability.minBitpool > sbc_capability.maxBitpool)
      return sbc_specifics;

    std::vector<SbcChannelMode> sbc_channel_modes =
        ExtractValuesFromBitmask<SbcChannelMode>(
            sbc_capability.channelModeBitmask, 0xf, true);
    std::vector<uint8_t> block_lengths = ExtractValuesFromBitmask<uint8_t>(
        sbc_capability.codecParametersBitmask, 0xf0, true);
    std::vector<uint8_t> subbands = ExtractValuesFromBitmask<uint8_t>(
        sbc_capability.codecParametersBitmask, 0x0c, true);
    std::vector<uint8_t> allocation_methods = ExtractValuesFromBitmask<uint8_t>(
        sbc_capability.codecParametersBitmask, 0x03, true);
    CodecConfigSpecific codec_specific;
    CodecConfigSpecific::SbcData sbc_data;
    for (auto sbc_channel_mode : sbc_channel_modes) {
      for (auto block_length : block_lengths) {
        for (auto subband : subbands) {
          for (auto allocation_method : allocation_methods) {
            sbc_data = {.channelMode = sbc_channel_mode,
                        .codecParameters = static_cast<uint8_t>(
                            block_length | subband | allocation_method),
                        .minBitpool = sbc_capability.minBitpool,
                        .maxBitpool = sbc_capability.maxBitpool};
            codec_specific.sbcData(sbc_data);
            sbc_specifics.push_back(codec_specific);
          }
        }
      }
    }
    return sbc_specifics;
  }

  std::vector<CodecConfigSpecific> GetAacCodecSpecificSupportedList() {
    CodecCapability::CodecSpecific::AacDataCapability aac_capability =
        codec_capability_.codecSpecific.aacDataCapability();
    std::vector<AacObjectType> aac_object_types =
        ExtractValuesFromBitmask<AacObjectType>(
            aac_capability.aacObjectTypeBitmask, 0xf0, true);
    std::vector<bool> is_vbr_supporteds(1, false);
    if (aac_capability.isVariableBitRateSupported)
      is_vbr_supporteds.push_back(true);
    std::vector<CodecConfigSpecific> aac_specifics;
    CodecConfigSpecific codec_specific;
    CodecConfigSpecific::AacData aac_data;
    for (auto aac_object_type : aac_object_types) {
      for (auto is_vbr_supported : is_vbr_supporteds) {
        aac_data = {.aacObjectType = aac_object_type,
                    .variableBitRateEnabled = is_vbr_supported};
        codec_specific.aacData(aac_data);
        aac_specifics.push_back(codec_specific);
      }
    }
    return aac_specifics;
  }

  std::vector<CodecConfigSpecific> GetLdacCodecSpecificSupportedList() {
    CodecCapability::CodecSpecific::LdacDataCapability ldac_capability =
        codec_capability_.codecSpecific.ldacDataCapability();
    std::vector<LdacChannelMode> ldac_channel_modes =
        ExtractValuesFromBitmask<LdacChannelMode>(
            ldac_capability.channelModeBitmask, 0x07, true);
    std::vector<uint8_t> bitrate_indexes = {0x00, 0x01, 0x02, 0x7f};
    std::vector<CodecConfigSpecific> ldac_specifics;
    CodecConfigSpecific codec_specific;
    CodecConfigSpecific::LdacData ldac_data;
    for (auto ldac_channel_mode : ldac_channel_modes) {
      for (auto bitrate_index : bitrate_indexes) {
        ldac_data = {.channelMode = ldac_channel_mode,
                     .bitrateIndex = bitrate_index};
        codec_specific.ldacData(ldac_data);
        ldac_specifics.push_back(codec_specific);
      }
    }
    return ldac_specifics;
  }

  CodecCapability codec_capability_;
};

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest, OpenA2dpHardwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpSbcHardwareSession) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  GetOffloadCodecCapabilitiesHelper(CodecType::SBC);
  if (codec_capability_.codecType == CodecType::UNKNOWN) return;
  std::vector<CodecConfiguration::PcmDataConfiguration> pcm_configs =
      GetPcmConfigurationSupportedList();
  ASSERT_NE(pcm_configs.size(), 0);
  std::vector<CodecConfigSpecific> sbc_specifics =
      GetSbcCodecSpecificSupportedList();
  ASSERT_NE(sbc_specifics.size(), 0);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  CodecConfiguration codec_config = {};
  codec_config.encodedDataConfiguration.codecType = CodecType::SBC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 328000;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  for (auto pcm_config : pcm_configs) {
    for (auto codec_specific : sbc_specifics) {
      codec_config.pcmDataConfiguration = pcm_config;
      codec_config.encodedDataConfiguration.codecSpecific = codec_specific;
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpAacHardwareSession) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  GetOffloadCodecCapabilitiesHelper(CodecType::AAC);
  if (codec_capability_.codecType == CodecType::UNKNOWN) return;
  std::vector<CodecConfiguration::PcmDataConfiguration> pcm_configs =
      GetPcmConfigurationSupportedList();
  ASSERT_NE(pcm_configs.size(), 0);
  std::vector<CodecConfigSpecific> aac_specifics =
      GetAacCodecSpecificSupportedList();
  ASSERT_NE(aac_specifics.size(), 0);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  CodecConfiguration codec_config = {};
  codec_config.encodedDataConfiguration.codecType = CodecType::AAC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 320000;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  for (auto pcm_config : pcm_configs) {
    for (auto codec_specific : aac_specifics) {
      codec_config.pcmDataConfiguration = pcm_config;
      codec_config.encodedDataConfiguration.codecSpecific = codec_specific;
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpAptxHardwareSession) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  for (auto codec_type : {CodecType::APTX, CodecType::APTX_HD}) {
    GetOffloadCodecCapabilitiesHelper(codec_type);
    if (codec_capability_.codecType == CodecType::UNKNOWN) return;
    std::vector<CodecConfiguration::PcmDataConfiguration> pcm_configs =
        GetPcmConfigurationSupportedList();
    ASSERT_NE(pcm_configs.size(), 0);
    auto hidl_cb = [](BluetoothAudioStatus status,
                      const DataMQ::Descriptor& dataMQ) {
      EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
      EXPECT_FALSE(dataMQ.isHandleValid());
    };
    CodecConfiguration codec_config = {};
    codec_config.encodedDataConfiguration.codecType = codec_type;
    codec_config.encodedDataConfiguration.encodedAudioBitrate =
        (codec_type == CodecType::APTX ? 352000 : 576000);
    codec_config.encodedDataConfiguration.peerMtu = 1005;
    codec_config.encodedDataConfiguration.isScmstEnabled = false;
    for (auto pcm_config : pcm_configs) {
      codec_config.pcmDataConfiguration = pcm_config;
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpLdacHardwareSession) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  GetOffloadCodecCapabilitiesHelper(CodecType::LDAC);
  if (codec_capability_.codecType == CodecType::UNKNOWN) return;
  std::vector<CodecConfiguration::PcmDataConfiguration> pcm_configs =
      GetPcmConfigurationSupportedList();
  ASSERT_NE(pcm_configs.size(), 0);
  std::vector<CodecConfigSpecific> ldac_specifics =
      GetLdacCodecSpecificSupportedList();
  ASSERT_NE(ldac_specifics.size(), 0);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  CodecConfiguration codec_config = {};
  codec_config.encodedDataConfiguration.codecType = CodecType::LDAC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 990000;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  for (auto pcm_config : pcm_configs) {
    for (auto codec_specific : ldac_specifics) {
      codec_config.pcmDataConfiguration = pcm_config;
      codec_config.encodedDataConfiguration.codecSpecific = codec_specific;
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidPcmConfig) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  codec_config.encodedDataConfiguration.codecType = CodecType::SBC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 32800;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  CodecConfigSpecific::SbcData sbc_data = {
      .channelMode = SbcChannelMode::JOINT_STEREO,
      .codecParameters = 0x15,  // block len 16 & subbands 8 & Loudness
      .minBitpool = 2,
      .maxBitpool = 53};
  codec_config.encodedDataConfiguration.codecSpecific.sbcData(sbc_data);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  for (auto sample_rate : a2dp_sample_rates) {
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : a2dp_bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : a2dp_channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // Test with invalid config only
          continue;
        }
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk());
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidEncodedConfig) {
  // Skip since it is not mandatory and seems to be unsupported
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  // FIXME: add more boundary tests
  for (auto codec_type : a2dp_codec_types) {
    codec_config.encodedDataConfiguration.codecType = codec_type;
    codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
    codec_config.encodedDataConfiguration.peerMtu = 0;
    codec_config.encodedDataConfiguration.isScmstEnabled = false;
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * openProvider HEARING_AID_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderHearingAidSoftwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    OpenProviderHelper(SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
                       true);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }
};

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       OpenHearingAidSoftwareProvider) {}

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       StartAndEndHearingAidSessionWithPossiblePcmConfig) {
  bool is_codec_config_valid;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    if (is_codec_config_valid) {
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_TRUE(dataMQ.isHandleValid());
      tempDataMQ.reset(new DataMQ(dataMQ));
    } else {
      EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
      EXPECT_FALSE(dataMQ.isHandleValid());
    }
  };
  CodecConfiguration codec_config;
  for (auto sample_rate : hearing_aid_sample_rates) {
    codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : hearing_aid_bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : hearing_aid_channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          is_codec_config_valid = true;
        } else {
          is_codec_config_valid = false;
        }
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk());
        if (is_codec_config_valid) {
          EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(
      BluetoothAudioHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  BluetoothAudioHidlEnvironment::Instance()->init(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
