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

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class EnvReverbSwContext final : public EffectContext {
  public:
    EnvReverbSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setErRoomLevel(int roomLevel) {
        if (roomLevel < Reverb::MIN_ROOM_LEVEL_MB || roomLevel > Reverb::MAX_ROOM_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid roomLevel: " << roomLevel;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room level
        mRoomLevel = roomLevel;
        return RetCode::SUCCESS;
    }
    int getErRoomLevel() const { return mRoomLevel; }

    RetCode setErRoomHfLevel(int roomHfLevel) {
        if (roomHfLevel < Reverb::MIN_ROOM_HF_LEVEL_MB ||
            roomHfLevel > Reverb::MAX_ROOM_HF_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid roomHfLevel: " << roomHfLevel;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room HF level
        mRoomHfLevel = roomHfLevel;
        return RetCode::SUCCESS;
    }
    int getErRoomHfLevel() const { return mRoomHfLevel; }

    RetCode setErDecayTime(int decayTime) {
        if (decayTime < Reverb::MIN_DECAY_TIME_MS || decayTime > Reverb::MAX_DECAY_TIME_MS) {
            LOG(ERROR) << __func__ << " invalid decayTime: " << decayTime;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay time
        mDecayTime = decayTime;
        return RetCode::SUCCESS;
    }
    int getErDecayTime() const { return mDecayTime; }

    RetCode setErDecayHfRatio(int decayHfRatio) {
        if (decayHfRatio < Reverb::MIN_DECAY_HF_RATIO_PM ||
            decayHfRatio > Reverb::MAX_DECAY_HF_RATIO_PM) {
            LOG(ERROR) << __func__ << " invalid decayHfRatio: " << decayHfRatio;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay HF ratio
        mDecayHfRatio = decayHfRatio;
        return RetCode::SUCCESS;
    }
    int getErDecayHfRatio() const { return mDecayHfRatio; }

    RetCode setErLevel(int level) {
        if (level < Reverb::MIN_LEVEL_MB || level > Reverb::MAX_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid level: " << level;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new level
        mLevel = level;
        return RetCode::SUCCESS;
    }
    int getErLevel() const { return mLevel; }

    RetCode setErDelay(int delay) {
        if (delay < Reverb::MIN_DELAY_MS || delay > Reverb::MAX_DELAY_MS) {
            LOG(ERROR) << __func__ << " invalid delay: " << delay;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new delay
        mDelay = delay;
        return RetCode::SUCCESS;
    }
    int getErDelay() const { return mDelay; }

    RetCode setErDiffusion(int diffusion) {
        if (diffusion < Reverb::MIN_DIFFUSION_PM || diffusion > Reverb::MAX_DIFFUSION_PM) {
            LOG(ERROR) << __func__ << " invalid diffusion: " << diffusion;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new diffusion
        mDiffusion = diffusion;
        return RetCode::SUCCESS;
    }
    int getErDiffusion() const { return mDiffusion; }

    RetCode setErDensity(int density) {
        if (density < Reverb::MIN_DENSITY_PM || density > Reverb::MAX_DENSITY_PM) {
            LOG(ERROR) << __func__ << " invalid density: " << density;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new density
        mDensity = density;
        return RetCode::SUCCESS;
    }
    int getErDensity() const { return mDensity; }

    RetCode setErBypass(bool bypass) {
        // TODO : Add implementation to apply new bypass
        mBypass = bypass;
        return RetCode::SUCCESS;
    }
    bool getErBypass() const { return mBypass; }

  private:
    int mRoomLevel = Reverb::MIN_ROOM_LEVEL_MB;       // Default room level
    int mRoomHfLevel = Reverb::MAX_ROOM_HF_LEVEL_MB;  // Default room hf level
    int mDecayTime = 1000;                            // Default decay time
    int mDecayHfRatio = 500;                          // Default decay hf ratio
    int mLevel = Reverb::MIN_LEVEL_MB;                // Default level
    int mDelay = 40;                                  // Default delay
    int mDiffusion = Reverb::MAX_DIFFUSION_PM;        // Default diffusion
    int mDensity = Reverb::MAX_DENSITY_PM;            // Default density
    bool mBypass = false;                             // Default bypass
};

class EnvReverbSw final : public EffectImpl {
  public:
    EnvReverbSw() { LOG(DEBUG) << __func__; }
    ~EnvReverbSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    std::shared_ptr<EffectContext> getContext() override;
    RetCode releaseContext() override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    const std::string kEffectName = "EnvReverbSw";
    std::shared_ptr<EnvReverbSwContext> mContext;
    /* capabilities */
    const int mMaxDecayTime = Reverb::MAX_DECAY_TIME_MS;
    const Reverb::Capability kCapability = {.maxDecayTimeMs = mMaxDecayTime};
    /* Effect descriptor */
    const Descriptor kDescriptor = {
            .common = {.id = {.type = kEnvReverbTypeUUID,
                              .uuid = kEnvReverbSwImplUUID,
                              .proxy = std::nullopt},
                       .flags = {.type = Flags::Type::INSERT,
                                 .insert = Flags::Insert::FIRST,
                                 .volume = Flags::Volume::CTRL},
                       .name = kEffectName,
                       .implementor = "The Android Open Source Project"},
            .capability = Capability::make<Capability::reverb>(kCapability)};

    ndk::ScopedAStatus getParameterReverb(const Reverb::Tag& tag, Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
