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
using ::android::hardware::bluetooth::audio::V2_0::Status;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

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

// The main test class for Bluetooth Audio HAL.
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

  sp<IBluetoothAudioProvidersFactory> providers_factory_;
};

TEST_F(BluetoothAudioProvidersFactoryHidlTest, GetProvidersFactoryService) {}

TEST_F(BluetoothAudioProvidersFactoryHidlTest, OpenProviderBySession) {
  for (auto session_type : session_types) {
    EXPECT_TRUE(
        providers_factory_
            ->openProvider(
                session_type,
                [session_type](Status status,
                               const sp<IBluetoothAudioProvider>& provider) {
                  if (session_type == SessionType::UNKNOWN) {
                    EXPECT_EQ(status, Status::FAILURE);
                    EXPECT_EQ(provider, nullptr);
                  } else {
                    // There is no SessionType to be mandatory
                    if (status == Status::SUCCESS) {
                      EXPECT_NE(provider, nullptr);
                    } else {
                      EXPECT_EQ(status, Status::FAILURE);
                      EXPECT_EQ(provider, nullptr);
                    }
                  }
                })
            .isOk());
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

    audio_provider_ = nullptr;
    EXPECT_TRUE(
        providers_factory_
            ->openProvider(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
                           [& audio_provider = this->audio_provider_](
                               Status status,
                               const sp<IBluetoothAudioProvider>& provider) {
                             if (status == Status::SUCCESS) {
                               ASSERT_NE(provider, nullptr);
                               audio_provider = std::move(provider);
                             } else {
                               EXPECT_EQ(status, Status::FAILURE);
                               EXPECT_EQ(provider, nullptr);
                             }
                           })
            .isOk());
    if (audio_provider_ != nullptr) {
      audio_port_ = new BluetoothAudioPort(*this);
      ASSERT_NE(audio_port_, nullptr);
    }
  }

  virtual void TearDown() override {
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
  }

  // A simple test implementation of IBluetoothAudioPort.
  class BluetoothAudioPort : public ::testing::VtsHalHidlTargetCallbackBase<
                                 BluetoothAudioProviderA2dpSoftwareHidlTest>,
                             public IBluetoothAudioPort {
    BluetoothAudioProviderA2dpSoftwareHidlTest& parent_;

   public:
    BluetoothAudioPort(BluetoothAudioProviderA2dpSoftwareHidlTest& parent)
        : parent_(parent) {}
    virtual ~BluetoothAudioPort() = default;

    Return<void> startStream() override {
      parent_.audio_provider_->streamStarted(Status::SUCCESS);
      return Void();
    }

    Return<void> suspendStream() override {
      parent_.audio_provider_->streamSuspended(Status::SUCCESS);
      return Void();
    }

    Return<void> stopStream() override { return Void(); }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
      _hidl_cb(Status::SUCCESS, 0, 0, {.tvSec = 0, .tvNSec = 0});
      return Void();
    }

    Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) {
      return Void();
    }
  };

  // audio_port is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth
  sp<IBluetoothAudioPort> audio_port_;

  // audio_provider is for the Bluetooth HAL to report session started/ended and
  // handled audio stream started / suspended
  sp<IBluetoothAudioProvider> audio_provider_;
};

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest, OpenA2dpSoftwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSession) {
  // skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  // FIXME: add all mandatory configs to test
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

  std::unique_ptr<DataMQ> tempDataMQ;
  EXPECT_TRUE(
      audio_provider_
          ->startSession(
              audio_port_, codec_config,
              [&tempDataMQ](Status status, const DataMQ::Descriptor& dataMQ) {
                ASSERT_EQ(status, Status::SUCCESS);
                ASSERT_TRUE(dataMQ.isHandleValid());
                tempDataMQ.reset(new DataMQ(dataMQ));
              })
          .isOk());
  EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());

  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSessionInvalidPcmConfig) {
  // skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  for (auto sample_rate : sample_rates) {
    CodecConfiguration codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // test with invalid config only
          continue;
        }
        EXPECT_TRUE(
            audio_provider_
                ->startSession(
                    audio_port_, codec_config,
                    [](Status status, const DataMQ::Descriptor& dataMQ) {
                      EXPECT_EQ(status,
                                Status::UNSUPPORTED_CODEC_CONFIGURATION);
                      EXPECT_FALSE(dataMQ.isHandleValid());
                    })
                .isOk());
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

    audio_provider_ = nullptr;
    EXPECT_TRUE(
        providers_factory_
            ->openProvider(SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH,
                           [& audio_provider = this->audio_provider_](
                               Status status,
                               const sp<IBluetoothAudioProvider>& provider) {
                             if (status == Status::SUCCESS) {
                               ASSERT_NE(provider, nullptr);
                               audio_provider = std::move(provider);
                             } else {
                               EXPECT_EQ(status, Status::FAILURE);
                               EXPECT_EQ(provider, nullptr);
                             }
                           })
            .isOk());
    if (audio_provider_ != nullptr) {
      audio_port_ = new BluetoothAudioPort(*this);
      ASSERT_NE(audio_port_, nullptr);
    }
  }

  virtual void TearDown() override {
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
  }

  // A simple test implementation of IBluetoothAudioPort.
  class BluetoothAudioPort : public ::testing::VtsHalHidlTargetCallbackBase<
                                 BluetoothAudioProviderA2dpHardwareHidlTest>,
                             public IBluetoothAudioPort {
    BluetoothAudioProviderA2dpHardwareHidlTest& parent_;

   public:
    BluetoothAudioPort(BluetoothAudioProviderA2dpHardwareHidlTest& parent)
        : parent_(parent) {}
    virtual ~BluetoothAudioPort() = default;

    Return<void> startStream() override {
      parent_.audio_provider_->streamStarted(Status::SUCCESS);
      return Void();
    }

    Return<void> suspendStream() override {
      parent_.audio_provider_->streamSuspended(Status::SUCCESS);
      return Void();
    }

    Return<void> stopStream() override { return Void(); }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
      _hidl_cb(Status::SUCCESS, 0, 0, {.tvSec = 0, .tvNSec = 0});
      return Void();
    }

    Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) {
      return Void();
    }
  };

  // audio_port is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth
  sp<IBluetoothAudioPort> audio_port_;

  // audio_provider is for the Bluetooth HAL to report session started/ended and
  // handled audio stream started / suspended
  sp<IBluetoothAudioProvider> audio_provider_;
};

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest, OpenA2dpHardwareProvider) {}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSession) {
  // skip since it is not mandatory and seems to be unsupported here
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

  EXPECT_TRUE(
      audio_provider_
          ->startSession(audio_port_, codec_config,
                         [](Status status, const DataMQ::Descriptor& dataMQ) {
                           ASSERT_EQ(status, Status::SUCCESS);
                           ASSERT_FALSE(dataMQ.isHandleValid());
                         })
          .isOk());

  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidPcmConfig) {
  // skip since it is not mandatory and seems to be unsupported here
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

  for (auto sample_rate : sample_rates) {
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // test with invalid config only
          continue;
        }
        EXPECT_TRUE(
            audio_provider_
                ->startSession(
                    audio_port_, codec_config,
                    [](Status status, const DataMQ::Descriptor& dataMQ) {
                      EXPECT_EQ(status,
                                Status::UNSUPPORTED_CODEC_CONFIGURATION);
                      EXPECT_FALSE(dataMQ.isHandleValid());
                    })
                .isOk());
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

TEST_F(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidEncodedConfig) {
  // skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

  // FIXME: add more boundary tests
  codec_config.encodedDataConfiguration.codecType = CodecType::UNKNOWN;
  codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
  codec_config.encodedDataConfiguration.peerMtu = 0;
  codec_config.encodedDataConfiguration.isScmstEnabled = false;

  EXPECT_TRUE(
      audio_provider_
          ->startSession(audio_port_, codec_config,
                         [](Status status, const DataMQ::Descriptor& dataMQ) {
                           EXPECT_EQ(status,
                                     Status::UNSUPPORTED_CODEC_CONFIGURATION);
                           EXPECT_FALSE(dataMQ.isHandleValid());
                         })
          .isOk());
  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

/**
 * openProvider HEARING_AID_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderHearingAidSoftwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();

    EXPECT_TRUE(
        providers_factory_
            ->openProvider(SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
                           [& audio_provider = this->audio_provider_](
                               Status status,
                               const sp<IBluetoothAudioProvider>& provider) {
                             if (status == Status::SUCCESS) {
                               ASSERT_NE(provider, nullptr);
                               audio_provider = std::move(provider);
                             } else {
                               EXPECT_EQ(status, Status::FAILURE);
                               EXPECT_EQ(provider, nullptr);
                             }
                           })
            .isOk());
    if (audio_provider_ != nullptr) {
      audio_port_ = new BluetoothAudioPort(*this);
      ASSERT_NE(audio_port_, nullptr);
    }
  }

  virtual void TearDown() override {
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
  }

  // A simple test implementation of IBluetoothAudioPort.
  class BluetoothAudioPort
      : public ::testing::VtsHalHidlTargetCallbackBase<
            BluetoothAudioProviderHearingAidSoftwareHidlTest>,
        public IBluetoothAudioPort {
    BluetoothAudioProviderHearingAidSoftwareHidlTest& parent_;

   public:
    BluetoothAudioPort(BluetoothAudioProviderHearingAidSoftwareHidlTest& parent)
        : parent_(parent) {}
    virtual ~BluetoothAudioPort() = default;

    Return<void> startStream() override {
      parent_.audio_provider_->streamStarted(Status::SUCCESS);
      return Void();
    }

    Return<void> suspendStream() override {
      parent_.audio_provider_->streamSuspended(Status::SUCCESS);
      return Void();
    }

    Return<void> stopStream() override { return Void(); }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
      _hidl_cb(Status::SUCCESS, 0, 0, {.tvSec = 0, .tvNSec = 0});
      return Void();
    }

    Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) {
      return Void();
    }
  };

  // audio_port is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth
  sp<IBluetoothAudioPort> audio_port_;

  // audio_provider is for the Bluetooth HAL to report session started/ended and
  // handled audio stream started / suspended
  sp<IBluetoothAudioProvider> audio_provider_;
};

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       OpenHearingAidSoftwareProvider) {}

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       StartAndEndHearingAidSoftwareSession) {
  // skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  CodecConfiguration codec_config = {};
  // we only support one config for now!
  codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_16000;
  codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
  codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

  std::unique_ptr<DataMQ> tempDataMQ;
  EXPECT_TRUE(
      audio_provider_
          ->startSession(
              audio_port_, codec_config,
              [&tempDataMQ](Status status, const DataMQ::Descriptor& dataMQ) {
                ASSERT_EQ(status, Status::SUCCESS);
                ASSERT_TRUE(dataMQ.isHandleValid());
                tempDataMQ.reset(new DataMQ(dataMQ));
              })
          .isOk());
  EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());

  EXPECT_TRUE(audio_provider_->endSession().isOk());
}

TEST_F(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       StartAndEndHearingAidSessionInvalidPcmConfig) {
  // skip since it is not mandatory and seems to be unsupported here
  if (audio_provider_ == nullptr) return;

  for (auto sample_rate : sample_rates) {
    CodecConfiguration codec_config = {};
    codec_config.pcmDataConfiguration.sampleRate = sample_rate;
    for (auto bits_per_sample : bits_per_samples) {
      codec_config.pcmDataConfiguration.bitsPerSample = bits_per_sample;
      for (auto channel_mode : channel_modes) {
        codec_config.pcmDataConfiguration.channelMode = channel_mode;
        if (sample_rate != SampleRate::RATE_UNKNOWN &&
            bits_per_sample != BitsPerSample::BITS_UNKNOWN &&
            channel_mode != ChannelMode::UNKNOWN) {
          // test with invalid config only
          continue;
        }
        EXPECT_TRUE(
            audio_provider_
                ->startSession(
                    audio_port_, codec_config,
                    [](Status status, const DataMQ::Descriptor& dataMQ) {
                      EXPECT_EQ(status,
                                Status::UNSUPPORTED_CODEC_CONFIGURATION);
                      EXPECT_FALSE(dataMQ.isHandleValid());
                    })
                .isOk());
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
