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
#include <cstdlib>
#include <memory>
#include <mutex>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>

#include "EffectContext.h"
#include "EffectThread.h"
#include "EffectTypes.h"
#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectThread.h"
#include "effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

class EffectImpl : public BnEffect, public EffectThread {
  public:
    EffectImpl() = default;
    virtual ~EffectImpl() = default;

    /**
     * Each effect implementation CAN override these methods if necessary
     * If you would like implement IEffect::open completely, override EffectImpl::open(), if you
     * want to keep most of EffectImpl logic but have a little customize, try override openImpl().
     * openImpl() will be called at the beginning of EffectImpl::open() without lock protection.
     *
     * Same for closeImpl().
     */
    virtual ndk::ScopedAStatus open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) override;
    virtual ndk::ScopedAStatus close() override;
    virtual ndk::ScopedAStatus command(CommandId id) override;

    virtual ndk::ScopedAStatus getState(State* state) override;
    virtual ndk::ScopedAStatus setParameter(const Parameter& param) override;
    virtual ndk::ScopedAStatus getParameter(const Parameter::Id& id, Parameter* param) override;

    virtual ndk::ScopedAStatus setParameterCommon(const Parameter& param);
    virtual ndk::ScopedAStatus getParameterCommon(const Parameter::Tag& tag, Parameter* param);

    /* Methods MUST be implemented by each effect instances */
    virtual ndk::ScopedAStatus getDescriptor(Descriptor* desc) = 0;
    virtual ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) = 0;
    virtual ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                                    Parameter::Specific* specific) = 0;

    virtual std::string getEffectName() = 0;
    virtual std::shared_ptr<EffectContext> getContext() = 0;
    // effectProcessImpl is in worker thread EffectThread, and should not hold mMutex lock.
    virtual IEffect::Status effectProcessImpl(float* in, float* out, int process) = 0;

    /*****************************************************************************
     **  Methods need to be call under mMutex lock.
     *****************************************************************************/
    /**
     * Command handling methods.
     */
    virtual ndk::ScopedAStatus commandStart_l() REQUIRES(mMutex) {
        return ndk::ScopedAStatus::ok();
    }
    virtual ndk::ScopedAStatus commandStop_l() REQUIRES(mMutex) { return ndk::ScopedAStatus::ok(); }
    virtual ndk::ScopedAStatus commandReset_l() REQUIRES(mMutex) {
        return ndk::ScopedAStatus::ok();
    }

    /**
     * Effect context methods must be implemented by each effect.
     * Each effect can derive from EffectContext and define its own context, but must upcast to
     * EffectContext for EffectImpl to use.
     */
    virtual std::shared_ptr<EffectContext> createContext_l(const Parameter::Common& common)
            REQUIRES(mMutex) = 0;
    virtual std::shared_ptr<EffectContext> getContext_l() REQUIRES(mMutex) = 0;
    virtual RetCode releaseContext_l() REQUIRES(mMutex) = 0;

  protected:
    /*
     * Lock is required to access state/context/buffer.
     */
    std::mutex mMutex;
    State mState GUARDED_BY(mMutex) = State::INIT;

    IEffect::Status status(binder_status_t status, size_t consumed, size_t produced);
    void cleanUp();
    // handle FMQ and call effect implemented effectProcessImpl.
    // To override this function, keep in mind this is running in EffectThread and should not take
    // mMutex, but use the EffectThread interfaces to make sure in sync.
    virtual void process() override;
};
}  // namespace aidl::android::hardware::audio::effect
