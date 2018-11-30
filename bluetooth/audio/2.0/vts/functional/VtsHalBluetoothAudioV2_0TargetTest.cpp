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
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioProvider;
using ::android::hardware::bluetooth::audio::V2_0::
    IBluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;

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
    bool is_cb_executed = false;
    BluetoothAudioStatus cb_status;
    auto hidl_cb = [&is_cb_executed, &cb_status,
                    &local_provider = this->audio_provider_](
                       BluetoothAudioStatus status,
                       const sp<IBluetoothAudioProvider>& provider) {
      // Test outside to make sure hidl_cb is really invoked
      is_cb_executed = true;
      cb_status = status;
      local_provider = provider;
    };
    auto hidl_retval = providers_factory_->openProvider(session_type, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
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
  bool is_cb_executed;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &is_cb_executed, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    is_cb_executed = true;
    if (is_codec_config_valid) {
      LOG(WARNING) << "[SEG_BT] " << __func__ << ":" << __LINE__ << " - ";
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_TRUE(dataMQ.isHandleValid());
      tempDataMQ.reset(new DataMQ(dataMQ));
    } else {
      LOG(WARNING) << "[SEG_BT] " << __func__ << ":" << __LINE__ << " - ";
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
        is_cb_executed = false;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
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
};

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest, OpenA2dpHardwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSession) {
  // Skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  // FIXME: add all mandatory configs to test
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;
  codec_config.encodedDataConfiguration.codecType = CodecType::SBC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 32800;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  CodecConfiguration::EncodedDataConfiguration::CodecSpecific::SbcData
      sbc_data = {
          .channelMode = SbcChannelMode::JOINT_STEREO,
          .codecParameters = 0x15,  // block len 16 & subbands 8 & Loudness
          .minBitpool = 2,
          .maxBitpool = 53};
  codec_config.encodedDataConfiguration.codecSpecific.sbcData(sbc_data);

  bool is_cb_executed = false;
  auto hidl_cb = [&is_cb_executed](BluetoothAudioStatus status,
                                   const DataMQ::Descriptor& dataMQ) {
    is_cb_executed = true;
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  auto hidl_retval =
      audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
  // HIDL calls should not be failed and callback has to be executed
  ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidPcmConfig) {
  // Skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  codec_config.encodedDataConfiguration.codecType = CodecType::SBC;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 32800;
  codec_config.encodedDataConfiguration.peerMtu = 1005;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;
  CodecConfiguration::EncodedDataConfiguration::CodecSpecific::SbcData
      sbc_data = {
          .channelMode = SbcChannelMode::JOINT_STEREO,
          .codecParameters = 0x15,  // block len 16 & subbands 8 & Loudness
          .minBitpool = 2,
          .maxBitpool = 53};
  codec_config.encodedDataConfiguration.codecSpecific.sbcData(sbc_data);
  bool is_cb_executed;
  auto hidl_cb = [&is_cb_executed](BluetoothAudioStatus status,
                                   const DataMQ::Descriptor& dataMQ) {
    is_cb_executed = true;
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
        is_cb_executed = false;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidEncodedConfig) {
  // Skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;
  bool is_cb_executed;
  auto hidl_cb = [&is_cb_executed](BluetoothAudioStatus status,
                                   const DataMQ::Descriptor& dataMQ) {
    is_cb_executed = true;
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  // FIXME: add more boundary tests
  for (auto codec_type : a2dp_codec_types) {
    codec_config.encodedDataConfiguration.codecType = codec_type;
    codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
    codec_config.encodedDataConfiguration.peerMtu = 0;
    codec_config.encodedDataConfiguration.isScmstEnabled = false;
    is_cb_executed = false;
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
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
  bool is_cb_executed;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &is_cb_executed, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    is_cb_executed = true;
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
        is_cb_executed = false;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk() && is_cb_executed);
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
