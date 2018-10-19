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

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::hidl_vec;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::sp;

using ::android::hardware::bluetooth::audio::V2_0::SessionType;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;

typedef MessageQueue<uint8_t, kSynchronizedReadWrite> DataMQ;
typedef ::android::hardware::bluetooth::audio::V2_0::Status BluetoothAudioStatus;

class BluetoothAudioDeathRecipient;

class BluetoothAudioProvider : public IBluetoothAudioProvider {
  public:
    BluetoothAudioProvider();
    // TODO
    //~BluetoothAudioProvider();

    Return<void> startSession(const sp<IBluetoothAudioPort>& hostIf,
                              const SessionType sessionType,
                              const CodecConfiguration& codecConfig,
                              startSession_cb _hidl_cb) override;
    Return<void> streamStarted(BluetoothAudioStatus status) override;
    Return<void> streamSuspended(BluetoothAudioStatus status) override;
    Return<void> endSession() override;

    hidl_vec<uint8_t>* getAudioStreamBuffer();
    const DataMQ::Descriptor* getAudioStreamFMQ();
    sp<IBluetoothAudioPort> getAudioStreamCtrl();

  protected:
    bool has_session_;
    sp<IBluetoothAudioPort> stack_iface_;

    hidl_vec<uint8_t> mStreamBuffer;
    std::unique_ptr<DataMQ> mDataMQ;

    sp<BluetoothAudioDeathRecipient> death_recipient_;
    std::function<void(sp<BluetoothAudioDeathRecipient>&)> unlink_cb_;
};

class BluetoothAudioDeathRecipient : public hidl_death_recipient {
  public:
    BluetoothAudioDeathRecipient(const sp<BluetoothAudioProvider> provider)
        : mProvider(provider), has_died_(true) {}

    virtual void serviceDied(
        uint64_t /* cookie */,
        const wp<::android::hidl::base::V1_0::IBase>& /* who */) {
      ALOGE("BluetoothAudioDeathRecipient::%s - BluetoothAudio Service died", __func__);
      has_died_ = true;
      mProvider->endSession();
    }

    bool getHasDied() const { return has_died_; }
    void setHasDied(bool has_died) { has_died_ = has_died; }

  private:
    sp<BluetoothAudioProvider> mProvider;
    bool has_died_;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BLUETOOTH_audio_V2_0_BluetoothAudioProvider_H_
