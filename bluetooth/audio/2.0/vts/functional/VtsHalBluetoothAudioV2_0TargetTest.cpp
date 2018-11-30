/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvidersFactory.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvider.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <utils/Log.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioProvider;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;
using ::android::hardware::bluetooth::audio::V2_0::Status;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SourceMetadata;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;

// Test environment for Bluetooth Audio HAL.
class BluetoothAudioHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static BluetoothAudioHidlEnvironment* Instance() {
        static BluetoothAudioHidlEnvironment* instance = new BluetoothAudioHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IBluetoothAudioProvidersFactory>(); }

   private:
    BluetoothAudioHidlEnvironment() {}
};

// The main test class for Bluetooth Audio HAL.
class BluetoothAudioHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        providers_factory =
            ::testing::VtsHalHidlTargetTestBase::getService<IBluetoothAudioProvidersFactory>(
                BluetoothAudioHidlEnvironment::Instance()->getServiceName<IBluetoothAudioProvidersFactory>());
        ASSERT_NE(providers_factory, nullptr);
    }

    virtual void TearDown() override {}

    sp<IBluetoothAudioProvidersFactory> providers_factory;
};

TEST_F(BluetoothAudioHidlTest, GetProvidersFactoryService) {
    LOG(INFO) << "Test the getProvidersFactoryService (called in SetUp)";
}

TEST_F(BluetoothAudioHidlTest, OpenProviderInvalidSession) {
    LOG(INFO) << "Test opening BluetoothAudioProvider with an invalid SessionType";
    EXPECT_TRUE(
        providers_factory->openProvider(
            SessionType::UNKNOWN,
            [&](Status status,
                const sp<IBluetoothAudioProvider>& provider __unused) {
                EXPECT_NE(status, Status::SUCCESS);
            }).isOk());
}

////////////////////////////////////////////////////////////////////////////////
//////////////// openProvider A2DP_SOFTWARE_ENCODING_DATAPATH or ///////////////
////////////////              A2DP_HARDWARE_OFFLOAD_DATAPATH     ///////////////
////////////////////////////////////////////////////////////////////////////////
class BluetoothAudioA2dpHidlTest : public BluetoothAudioHidlTest {
   public:
    virtual void SetUp() override {
        BluetoothAudioHidlTest::SetUp();

        audio_provider = nullptr;
        EXPECT_TRUE(
            providers_factory->openProvider(
                SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
                [&](Status status, const sp<IBluetoothAudioProvider>& provider) {
                    if (status == Status::SUCCESS) {
                        audio_provider = std::move(provider);
                        is_offloading = false;
                    }
                }).isOk());
        if (audio_provider == nullptr) {
            SCOPED_TRACE("Try to open A2DP_HARDWARE_OFFLOAD_DATAPATH");
            EXPECT_TRUE(
                providers_factory->openProvider(
                    SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH,
                    [&](Status status, const sp<IBluetoothAudioProvider>& provider)
                    {
                        if (status == Status::SUCCESS) {
                            audio_provider = std::move(provider);
                            is_offloading = true;
                        }
                    }).isOk());
        }

        ASSERT_NE(audio_provider, nullptr);

        audio_port = new BluetoothAudioPort(*this);
        ASSERT_NE(audio_port, nullptr);
    }

    virtual void TearDown() override {}

    // A simple test implementation of IBluetoothAudioPort.
    class BluetoothAudioPort
        : public ::testing::VtsHalHidlTargetCallbackBase<BluetoothAudioA2dpHidlTest>,
          public IBluetoothAudioPort {
        BluetoothAudioA2dpHidlTest& parent_;

       public:
        BluetoothAudioPort(BluetoothAudioA2dpHidlTest& parent) : parent_(parent) {}
        virtual ~BluetoothAudioPort() = default;

        Return<void> startStream() override {
            parent_.audio_provider->streamStarted(Status::SUCCESS);
            return Void();
        }

        Return<void> suspendStream() override {
            parent_.audio_provider->streamSuspended(Status::SUCCESS);
            return Void();
        }

        Return<void> stopStream() override { return Void(); }

        Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
            _hidl_cb(Status::SUCCESS, { .tvSec = 0, .tvNSec = 0 }, 0,
                     { .tvSec = 0, .tvNSec = 0 });
            return Void();
        }

        Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) { return Void(); }
    };

    // audio_port is for the Audio HAL to send stream start/suspend/stop commands to Bluetooth
    sp<IBluetoothAudioPort> audio_port;

    // audio_provider is for the Bluetooth HAL to report session started/ended and handled audio
    // stream started / suspended
    sp<IBluetoothAudioProvider> audio_provider;

    bool is_offloading;
};

TEST_F(BluetoothAudioA2dpHidlTest, OpenA2dpProvider) {
    LOG(INFO) << "Test the openProvider for A2DP (called in SetUp)";
}

TEST_F(BluetoothAudioA2dpHidlTest, StartAndEndA2dpSession) {
    LOG(INFO) << "Test the startSession and endSession works correctly";

    CodecConfiguration codec_config;
    codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
    codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
    codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

    if (is_offloading) {
        // parameters used when offloading data path
        codec_config.encodedDataConfiguration.codecType = CodecType::SBC;
        codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x0004E200;
        codec_config.encodedDataConfiguration.peerMtu = 1000;
        codec_config.encodedDataConfiguration.isScmstEnabled = false;
        codec_config.encodedDataConfiguration.codecSpecific.sbcData.channelMode = SbcChannelMode::JOINT_STEREO;
        // block len 16 / subbands 8 / Loudness
        codec_config.encodedDataConfiguration.codecSpecific.sbcData.codecParameters = 0x15;
        codec_config.encodedDataConfiguration.codecSpecific.sbcData.minBitpool = 2;
        codec_config.encodedDataConfiguration.codecSpecific.sbcData.maxBitpool = 53;
    } else {
        // unused parameters when legacy data path
        codec_config.encodedDataConfiguration.codecType = CodecType::UNKNOWN;
        codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
        codec_config.encodedDataConfiguration.peerMtu = 0;
        codec_config.encodedDataConfiguration.isScmstEnabled = false;
    }

    std::unique_ptr<DataMQ> tempDataMQ;
    EXPECT_TRUE(
        audio_provider->startSession(
            audio_port, codec_config,
            [&](Status status, const DataMQ::Descriptor& dataMQ) {
                if (status == Status::SUCCESS) {
                    tempDataMQ.reset(new DataMQ(dataMQ));
                }
            }).isOk());
    EXPECT_TRUE(tempDataMQ != nullptr);
    EXPECT_TRUE(tempDataMQ->isValid());

    EXPECT_TRUE(audio_provider->endSession().isOk());
}

TEST_F(BluetoothAudioA2dpHidlTest, StartAndEndA2dpSessionInvalidPcmConfig) {
    LOG(INFO) << "Test calling startSession by an invalid PCM configuration";

    CodecConfiguration codec_config;
    codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_UNKNOWN;
    codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_UNKNOWN;
    codec_config.pcmDataConfiguration.channelMode = ChannelMode::UNKNOWN;

    std::unique_ptr<DataMQ> tempDataMQ;
    EXPECT_TRUE(
        audio_provider->startSession(
            audio_port, codec_config,
            [&](Status status, const DataMQ::Descriptor& dataMQ) {
                EXPECT_NE(status, Status::UNSUPPORTED_CODEC_CONFIGURATION);
                tempDataMQ.reset(new DataMQ(dataMQ));
            }).isOk());
    EXPECT_TRUE(tempDataMQ == nullptr || !tempDataMQ->isValid());
    EXPECT_TRUE(audio_provider->endSession().isOk());
}

TEST_F(BluetoothAudioA2dpHidlTest, StartAndEndA2dpSessionInvalidEncodedConfig) {
    LOG(INFO) << "Test calling startSession by an invalid encoded configuration";

    if (is_offloading) {
        CodecConfiguration codec_config;
        codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
        codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
        codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

        codec_config.encodedDataConfiguration.codecType = CodecType::UNKNOWN;
        codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
        codec_config.encodedDataConfiguration.peerMtu = 0;
        codec_config.encodedDataConfiguration.isScmstEnabled = false;

        std::unique_ptr<DataMQ> tempDataMQ;
        EXPECT_TRUE(
            audio_provider->startSession(
                audio_port, codec_config,
                [&](Status status, const DataMQ::Descriptor& dataMQ) {
                    EXPECT_NE(status, Status::UNSUPPORTED_CODEC_CONFIGURATION);
                    tempDataMQ.reset(new DataMQ(dataMQ));
                }).isOk());
        EXPECT_TRUE(tempDataMQ == nullptr || !tempDataMQ->isValid());
        EXPECT_TRUE(audio_provider->endSession().isOk());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////// openProvider HEARING_AID_SOFTWARE_ENCODING_DATAPATH /////////////
////////////////////////////////////////////////////////////////////////////////
class BluetoothAudioHearingAidHidlTest : public BluetoothAudioHidlTest {
   public:
    virtual void SetUp() override {
        BluetoothAudioHidlTest::SetUp();

        EXPECT_TRUE(
            providers_factory->openProvider(
                SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
                [&](Status status, const sp<IBluetoothAudioProvider>& provider) {
                    if (status == Status::SUCCESS) {
                        audio_provider = std::move(provider);
                    }
                }).isOk());
        ASSERT_NE(audio_provider, nullptr);

        audio_port = new BluetoothAudioPort(*this);
        ASSERT_NE(audio_port, nullptr);
    }

    virtual void TearDown() override {}

    // A simple test implementation of IBluetoothAudioPort.
    class BluetoothAudioPort
        : public ::testing::VtsHalHidlTargetCallbackBase<BluetoothAudioHearingAidHidlTest>,
          public IBluetoothAudioPort {
        BluetoothAudioHearingAidHidlTest& parent_;

       public:
        BluetoothAudioPort(BluetoothAudioHearingAidHidlTest& parent) : parent_(parent) {}
        virtual ~BluetoothAudioPort() = default;

        Return<void> startStream() override {
            parent_.audio_provider->streamStarted(Status::SUCCESS);
            return Void();
        }

        Return<void> suspendStream() override {
            parent_.audio_provider->streamSuspended(Status::SUCCESS);
            return Void();
        }

        Return<void> stopStream() override { return Void(); }

        Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
            _hidl_cb(Status::SUCCESS, { .tvSec = 0, .tvNSec = 0 }, 0,
                     { .tvSec = 0, .tvNSec = 0 });
            return Void();
        }

        Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) { return Void(); }
    };

    // audio_port is for the Audio HAL to send stream start/suspend/stop commands to Bluetooth
    sp<IBluetoothAudioPort> audio_port;

    // audio_provider is for the Bluetooth HAL to report session started/ended and handled audio
    // stream started / suspended
    sp<IBluetoothAudioProvider> audio_provider;
};

TEST_F(BluetoothAudioHearingAidHidlTest, OpenHearingAidProvider) {
    LOG(INFO) << "Test the openProvider for HEARING_AID_SOFTWARE_ENCODING_DATAPATH (called in SetUp)";
}

TEST_F(BluetoothAudioHearingAidHidlTest, StartAndEndHearingAidSession) {
    LOG(INFO) << "Test the startSession and endSession works correctly";

    CodecConfiguration codec_config;
    codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_44100;
    codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_16;
    codec_config.pcmDataConfiguration.channelMode = ChannelMode::STEREO;

    // unused parameters
    codec_config.encodedDataConfiguration.codecType = CodecType::UNKNOWN;
    codec_config.encodedDataConfiguration.encodedAudioBitrate = 0x00000000;
    codec_config.encodedDataConfiguration.peerMtu = 0;
    codec_config.encodedDataConfiguration.isScmstEnabled = false;

    std::unique_ptr<DataMQ> tempDataMQ;
    EXPECT_TRUE(
        audio_provider->startSession(
            audio_port, codec_config,
            [&](Status status, const DataMQ::Descriptor& dataMQ) {
                if (status == Status::SUCCESS) {
                    tempDataMQ.reset(new DataMQ(dataMQ));
                }
            }).isOk());
    EXPECT_TRUE(tempDataMQ != nullptr);
    EXPECT_TRUE(tempDataMQ->isValid());

    EXPECT_TRUE(audio_provider->endSession().isOk());
}

TEST_F(BluetoothAudioHearingAidHidlTest, StartAndEndHearingAidSessionInvalidPcmConfig) {
    LOG(INFO) << "Test calling startSession by an invalid PCM configuration";

    CodecConfiguration codec_config;
    codec_config.pcmDataConfiguration.sampleRate = SampleRate::RATE_UNKNOWN;
    codec_config.pcmDataConfiguration.bitsPerSample = BitsPerSample::BITS_UNKNOWN;
    codec_config.pcmDataConfiguration.channelMode = ChannelMode::UNKNOWN;

    std::unique_ptr<DataMQ> tempDataMQ;
    EXPECT_TRUE(
        audio_provider->startSession(
            audio_port, codec_config,
            [&](Status status, const DataMQ::Descriptor& dataMQ) {
                EXPECT_NE(status, Status::UNSUPPORTED_CODEC_CONFIGURATION);
                tempDataMQ.reset(new DataMQ(dataMQ));
            }).isOk());
    EXPECT_TRUE(tempDataMQ == nullptr || !tempDataMQ->isValid());
    EXPECT_TRUE(audio_provider->endSession().isOk());
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(BluetoothAudioHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    BluetoothAudioHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
