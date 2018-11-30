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
constexpr SampleRate sample_rates[2] = {SampleRate::RATE_UNKNOWN,
                                        SampleRate::RATE_44100};
constexpr BitsPerSample bits_per_samples[2] = {BitsPerSample::BITS_UNKNOWN,
                                               BitsPerSample::BITS_16};
constexpr ChannelMode channel_modes[2] = {ChannelMode::UNKNOWN,
                                          ChannelMode::STEREO};
constexpr CodecType codec_types[6] = {CodecType::UNKNOWN, CodecType::SBC,
                                      CodecType::AAC,     CodecType::APTX,
                                      CodecType::APTX_HD, CodecType::LDAC};
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

  void OpenProvider(const SessionType& session_type, bool is_mandatory) {
    // Tool error if failed
    ASSERT_NE(providers_factory_, nullptr);

    BluetoothAudioStatus expect_status;
    if (is_mandatory && session_type != SessionType::UNKNOWN) {
      expect_status = BluetoothAudioStatus::FAILURE;
    } else {
      expect_status = BluetoothAudioStatus::SUCCESS;
    }
    auto hidl_cb = [&expect_status, &local_provider = this->audio_provider_](
                       BluetoothAudioStatus status,
                       const sp<IBluetoothAudioProvider>& provider) {
      // Test outside to make sure hidl_cb is really invoked
      expect_status = status;
      local_provider = provider;
    };
    auto hidl_retval = providers_factory_->openProvider(session_type, hidl_cb);
    // HIDL calls should never be failed
    ASSERT_TRUE(hidl_retval.isOk());
    if (is_mandatory && session_type != SessionType::UNKNOWN) {
      ASSERT_EQ(expect_status, BluetoothAudioStatus::SUCCESS);
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = new BluetoothAudioPort(*this);
      ASSERT_NE(audio_port_, nullptr);
    } else if (session_type != SessionType::UNKNOWN &&
               expect_status == BluetoothAudioStatus::SUCCESS) {
      // Optional session_type seems to be supported by DUT
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = new BluetoothAudioPort(*this);
      ASSERT_NE(audio_port_, nullptr);
    } else {
      // DUT does not support this session_type or session_type is unknown
      ASSERT_EQ(expect_status, BluetoothAudioStatus::FAILURE);
      ASSERT_EQ(audio_provider_, nullptr);
    }
    // Well to keep testing further
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
  BluetoothAudioStatus expect_status;
  auto hidl_cb = [&expect_status, &local_provider = this->audio_provider_](
                     BluetoothAudioStatus status,
                     const sp<IBluetoothAudioProvider>& provider) {
    expect_status = status;
    local_provider = provider;
  };
  for (auto session_type : session_types) {
    audio_provider_ = nullptr;
    if (session_type == SessionType::UNKNOWN ||
        session_type != SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
      expect_status = BluetoothAudioStatus::SUCCESS;
    } else {
      expect_status = BluetoothAudioStatus::FAILURE;
    }
    auto hidl_retval = providers_factory_->openProvider(session_type, hidl_cb);
    // HIDL calls should never be failed
    ASSERT_TRUE(hidl_retval.isOk());
    if (session_type == SessionType::UNKNOWN) {
      EXPECT_EQ(expect_status, BluetoothAudioStatus::FAILURE);
      EXPECT_EQ(audio_provider_, nullptr);
    } else if (session_type != SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
      // All software encodings are mandatory
      EXPECT_EQ(expect_status, BluetoothAudioStatus::SUCCESS);
      EXPECT_NE(audio_provider_, nullptr);
    } else if (expect_status == BluetoothAudioStatus::SUCCESS) {
      // A2DP offloading is supported by DUT
      EXPECT_NE(audio_provider_, nullptr);
    } else {
      // A2DP offloading is optional and seems DUT does not support it
      EXPECT_EQ(expect_status, BluetoothAudioStatus::FAILURE);
      EXPECT_EQ(audio_provider_, nullptr);
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
    OpenProvider(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH, true);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }
};

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest, OpenA2dpSoftwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSession) {
  CodecConfiguration codec_config = {};
  // FIXME: add all mandatory configs to test
  // SBC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(MONO|STEREO)
  // AAC: mSampleRate:(44100), mBitsPerSample:(16), mChannelMode:(STEREO)
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto expect_status = BluetoothAudioStatus::FAILURE;
  auto hidl_cb = [&tempDataMQ, &expect_status](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
    ASSERT_TRUE(dataMQ.isHandleValid());
    expect_status = status;
    tempDataMQ.reset(new DataMQ(dataMQ));
  };
  auto hidl_retval =
      audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
  // HIDL calls should never be failed
  ASSERT_TRUE(hidl_retval.isOk());
  // Make sure hidl_cb is really invoked before further testing
  ASSERT_NE(expect_status, BluetoothAudioStatus::FAILURE);
  EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSessionInvalidPcmConfig) {
  BluetoothAudioStatus expect_status;
  auto hidl_cb = [&expect_status](BluetoothAudioStatus status,
                                  const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
    expect_status = status;
  };
  CodecConfiguration codec_config;
  for (auto sample_rate : sample_rates) {
    codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // Test with invalid config only
          continue;
        }
        expect_status = BluetoothAudioStatus::SUCCESS;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should never be failed
        ASSERT_TRUE(hidl_retval.isOk());
        // Make sure hidl_cb is really invoked before further testing
        ASSERT_NE(expect_status, BluetoothAudioStatus::SUCCESS);
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
    OpenProvider(SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH, false);
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

  auto expect_status = BluetoothAudioStatus::FAILURE;
  auto hidl_cb = [&expect_status](BluetoothAudioStatus status,
                                  const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
    expect_status = status;
  };
  auto hidl_retval =
      audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
  // HIDL calls should never be failed
  ASSERT_TRUE(hidl_retval.isOk());
  // Make sure hidl_cb is really invoked before further testing
  ASSERT_NE(expect_status, BluetoothAudioStatus::FAILURE);
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
  BluetoothAudioStatus expect_status;
  auto hidl_cb = [&expect_status](BluetoothAudioStatus status,
                                  const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
    expect_status = status;
  };
  for (auto sample_rate : sample_rates) {
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // Test with invalid config only
          continue;
        }
        expect_status = BluetoothAudioStatus::SUCCESS;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should never be failed
        ASSERT_TRUE(hidl_retval.isOk());
        // Make sure hidl_cb is really invoked before further testing
        ASSERT_NE(expect_status, BluetoothAudioStatus::SUCCESS);
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
  BluetoothAudioStatus expect_status;
  auto hidl_cb = [&expect_status](BluetoothAudioStatus status,
                                  const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
    expect_status = status;
  };
  // FIXME: add more boundary tests
  for (auto codec_type : codec_types) {
    codec_config.encodedDataConfiguration.codecType = codec_type;
    codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
    codec_config.encodedDataConfiguration.peerMtu = 0;
    codec_config.encodedDataConfiguration.isScmstEnabled = false;
    expect_status = BluetoothAudioStatus::SUCCESS;
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
    // HIDL calls should never be failed
    ASSERT_TRUE(hidl_retval.isOk());
    // Make sure hidl_cb is really invoked before further testing
    ASSERT_NE(expect_status, BluetoothAudioStatus::SUCCESS);
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
    OpenProvider(SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH, true);
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
       StartAndEndHearingAidSoftwareSession) {
  CodecConfiguration codec_config = {};
  // we only support one config for now!
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_16000;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

  std::unique_ptr<DataMQ> tempDataMQ;
  auto expect_status = BluetoothAudioStatus::FAILURE;
  auto hidl_cb = [&tempDataMQ, &expect_status](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
    ASSERT_TRUE(dataMQ.isHandleValid());
    expect_status = status;
    tempDataMQ.reset(new DataMQ(dataMQ));
  };
  auto hidl_retval =
      audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
  // HIDL calls should never be failed
  ASSERT_TRUE(hidl_retval.isOk());
  // Make sure hidl_cb is really invoked before further testing
  ASSERT_NE(expect_status, BluetoothAudioStatus::FAILURE);
  EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       StartAndEndHearingAidSessionInvalidPcmConfig) {
  BluetoothAudioStatus expect_status;
  auto hidl_cb = [&expect_status](BluetoothAudioStatus status,
                                  const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
    EXPECT_FALSE(dataMQ.isHandleValid());
    expect_status = status;
  };
  CodecConfiguration codec_config;
  for (auto sample_rate : sample_rates) {
    codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // Test with invalid config only
          continue;
        }
        expect_status = BluetoothAudioStatus::SUCCESS;
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, codec_config, hidl_cb);
        // HIDL calls should never be failed
        ASSERT_TRUE(hidl_retval.isOk());
        // Make sure hidl_cb is really invoked before further testing
        ASSERT_NE(expect_status, BluetoothAudioStatus::SUCCESS);
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
