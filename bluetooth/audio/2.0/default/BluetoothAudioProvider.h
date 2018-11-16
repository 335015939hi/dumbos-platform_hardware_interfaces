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

#ifndef ANDROID_HARDWARE_BLUETOOTH_AUDIO_V2_0_BluetoothAudioProvider_H_
#define ANDROID_HARDWARE_BLUETOOTH_AUDIO_V2_0_BluetoothAudioProvider_H_

#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvider.h>

#include <unordered_map>
#include <mutex>

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hardware/audio.h>

/*****************************************************************************
 * Constants & Macros
 *****************************************************************************/
#define OBSERVERS_SIZE_                           0x100
#define OBSERVERS_SESSION_TYPE_MASK_              0xFF00
#define OBSERVERS_SESSION_TYPE_OFFSET_            8
#define OBSERVERS_GET_SESSION_TYPE(x)                                              \
  static_cast<SessionType>((x >> OBSERVERS_SESSION_TYPE_OFFSET_) & 0x00FF)
#define OBSERVERS_CTRL_KEY_UNDEF                                                   \
  (static_cast<uint16_t>(SessionType::UNKNOWN) << OBSERVERS_SESSION_TYPE_OFFSET_)

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using std::shared_ptr;

using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;
using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::sp;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;
typedef ::android::hardware::bluetooth::audio::V2_0::Status BluetoothAudioStatus;
typedef ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration::PcmDataConfiguration PcmDataConfiguration;

class BluetoothAudioDeathRecipient;

class BluetoothAudioProvider : public IBluetoothAudioProvider {
  public:
    BluetoothAudioProvider();
    ~BluetoothAudioProvider() = default;

    virtual bool build(const SessionType& sessionType);

    Return<void> startSession(const sp<IBluetoothAudioPort>& hostIf,
                              const CodecConfiguration& codecConfig,
                              startSession_cb _hidl_cb) override;
    Return<void> streamStarted(BluetoothAudioStatus status) override;
    Return<void> streamSuspended(BluetoothAudioStatus status) override;
    Return<void> endSession() override;

    // Extra APIs for Audio HW module: bluetooth_audio
    const DataMQ::Descriptor* getStreamDataFMQ();
    const PcmDataConfiguration& getStreamPcmDataConfig();
    const sp<IBluetoothAudioPort> getAssociatedPortCtrl();

    virtual bool getStreamPresentationPosition(timespec &remote_delay_report,
                                               uint64_t &total_bytes_readed,
                                               timespec &data_position);
    virtual void updateStreamTracksMetadata(
        const struct source_metadata *source_metadata);

    uint16_t registerControlResultCback(
        std::function<void(const uint16_t&, const BluetoothAudioStatus&)>&,
        std::function<void(const uint16_t&)>&);
    Return<void> unregisterControlResultCback(uint16_t& ctrl_Key);

    static const PcmDataConfiguration kInvalidPcmConfiguration;

  protected:
    // audio data queue for software encoding (legacy)
    std::unique_ptr<DataMQ> mDataMQ;

    SessionType session_type_;
    CodecConfiguration codec_config_;
    sp<IBluetoothAudioPort> stack_iface_;
    bool has_session_;

    // callback functions report BluetoothAudioPort state to bluetooth_audio
    struct PortStateCback {
      uint16_t ctrl_key;   // 8 bits: SessionType + 8 bits: map's key
      std::function<void(const uint16_t&, const BluetoothAudioStatus&)> ctrl_res_cb_;
      std::function<void(const uint16_t&)> session_changed_cb_;
    };
    std::unordered_map<uint16_t, shared_ptr<struct PortStateCback>> observers_;

    sp<BluetoothAudioDeathRecipient> death_recipient_;
    std::function<void(sp<BluetoothAudioDeathRecipient>&)> unlink_cb_;

  private:
    std::mutex internal_mutex_;
};

class BluetoothAudioDeathRecipient : public hidl_death_recipient {
  public:
    BluetoothAudioDeathRecipient(const sp<BluetoothAudioProvider> provider)
        : provider_(provider), has_died_(true) {}

    virtual void serviceDied(
        uint64_t /* cookie */,
        const wp<::android::hidl::base::V1_0::IBase>& /* who */) {
      ALOGE("BluetoothAudioDeathRecipient::%s - BluetoothAudio Service died", __func__);
      has_died_ = true;
      provider_->endSession();
    }

    bool getHasDied() const { return has_died_; }
    void setHasDied(bool has_died) { has_died_ = has_died; }

  private:
    sp<BluetoothAudioProvider> provider_;
    bool has_died_;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BLUETOOTH_audio_V2_0_BluetoothAudioProvider_H_
