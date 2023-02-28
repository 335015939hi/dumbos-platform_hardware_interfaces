/*
 * Copyright (C) 2023 The Android Open Source Project
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

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <utils/RefBase.h>
#include <utils/Singleton.h>
#include <utils/StrongPointer.h>

extern "C" {
#include <tinyalsa/mixer.h>
}

namespace aidl::android::hardware::audio::core::usb {

class MixerControl {
  public:
    explicit MixerControl(struct mixer_ctl* ctl);

    unsigned int getNumValues() const;
    int getMaxValue() const;
    int getMinValue() const;
    int getArray(void* array, size_t count);
    int setArray(const void* array, size_t count);

  private:
    int getArray_l() const REQUIRES(mLock);
    int setArray_l(const void* array, size_t count) REQUIRES(mLock);

    struct mixer_ctl* mCtl;
    std::mutex mLock;
    unsigned int mNumValues;
    int mMinValue;
    int mMaxValue;
};

class AlsaMixer {
  public:
    explicit AlsaMixer(struct mixer* mixer);

    virtual ~AlsaMixer();

    bool isValid() const { return mMixer != nullptr; }

    ::android::status_t setMasterMuted(bool muted);
    ::android::status_t setMasterVolume(float volume);
    ::android::status_t setVolumes(std::vector<float> volumes);
    std::optional<std::vector<float>> getVolumes() const;

  private:
    enum Control {
        MASTER_SWITCH,
        MASTER_VOLUME,
        HW_VOLUME,
    };
    using ControlNamesAndExpectedCtlType = std::pair<std::string, enum mixer_ctl_type>;
    static const std::map<Control, std::vector<ControlNamesAndExpectedCtlType>> kPossibleControls;

    struct mixer* mMixer;
    // `mMixerControls` will only be initialized in constructor. After that, it wil only be
    // read but not be modified.
    std::map<Control, std::shared_ptr<MixerControl>> mMixerControls;
};

class UsbAlsaMixerControl : public virtual ::android::RefBase,
                            private ::android::Singleton<UsbAlsaMixerControl> {
  public:
    static ::android::sp<UsbAlsaMixerControl> getInstance();

    void setDeviceConnectionState(int card, bool masterMuted, float masterVolume, bool connected);

    // Master volume settings will be applied to all sound cards, it is only set by the
    // USB module.
    void setMasterMuted(bool muted);
    void setMasterVolume(float volume);
    // The volume settings can be different on sound cards. It is controlled by streams.
    ::android::status_t setVolumes(int card, std::vector<float> volumes);
    std::optional<std::vector<float>> getVolumes(int card);

  private:
    friend class Singleton<UsbAlsaMixerControl>;
    UsbAlsaMixerControl();
    std::shared_ptr<AlsaMixer> getAlsaMixer(int card);
    std::map<int, std::shared_ptr<AlsaMixer>> getAlsaMixers();

    ::android::sp<UsbAlsaMixerControl> mSelf;  // Cache singleton instance to live forever.

    std::mutex mLock;
    std::map<int, std::shared_ptr<AlsaMixer>> mMixerControls GUARDED_BY(mLock);
};

}  // namespace aidl::android::hardware::audio::core::usb
