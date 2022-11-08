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

class BassBoostSwContext final : public EffectContext {
  public:
    BassBoostSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setBbStrengthPm(int strength) {
        if (strength < MIN_STRENGTH || strength > MAX_STRENGTH) {
            LOG(ERROR) << __func__ << " invalid strength ";
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new strength
        mStrength = strength;
        return RetCode::SUCCESS;
    }
    int getBbStrengthPm() const { return mStrength; }

  private:
    static const int MAX_STRENGTH = 1000;
    static const int MIN_STRENGTH = 0;
    int mStrength;
};

class BassBoostSw final : public EffectImpl {
  public:
    BassBoostSw() { LOG(DEBUG) << __func__; }
    ~BassBoostSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    IEffect::Status effectProcessImpl(float* in, float* out, int process) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    RetCode releaseContext() override;

  private:
    std::shared_ptr<BassBoostSwContext> mContext;
    /* capabilities */
    const bool mStrengthSupported = true;
    const BassBoost::Capability kCapability = {.strengthSupported = mStrengthSupported};
    /* Effect descriptor */
    const Descriptor kDescriptor = {
            .common = {.id = {.type = kBassBoostTypeUUID,
                              .uuid = kBassBoostSwImplUUID,
                              .proxy = std::nullopt},
                       .flags = {.type = Flags::Type::INSERT,
                                 .insert = Flags::Insert::FIRST,
                                 .volume = Flags::Volume::CTRL},
                       .name = "BassBoostSw",
                       .implementor = "The Android Open Source Project"},
            .capability = Capability::make<Capability::bassBoost>(kCapability)};

    ndk::ScopedAStatus getParameterBassBoost(const BassBoost::Tag& tag,
                                             Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
