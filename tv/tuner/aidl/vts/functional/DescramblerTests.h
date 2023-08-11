/*
 * Copyright 2021 The Android Open Source Project
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

#include <gtest/gtest.h>
#include <log/log.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

#include <android/hardware/cas/1.0/types.h>
#include <android/hardware/cas/1.2/ICas.h>
#include <android/hardware/cas/1.2/ICasListener.h>
#include <android/hardware/cas/1.2/IMediaCasService.h>
#include <android/hardware/cas/1.2/types.h>

#include <aidl/android/hardware/cas/BnCasListener.h>
#include <aidl/android/hardware/cas/ICas.h>
#include <aidl/android/hardware/cas/IMediaCasService.h>
#include <aidl/android/hardware/cas/Status.h>

#include <aidl/android/hardware/tv/tuner/IDescrambler.h>
#include <aidl/android/hardware/tv/tuner/IDvr.h>
#include <aidl/android/hardware/tv/tuner/IDvrCallback.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>

using android::Condition;
using android::Mutex;

using android::sp;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using ICasHidl = android::hardware::cas::V1_2::ICas;
using ICasListenerHidl = android::hardware::cas::V1_2::ICasListener;
using IMediaCasServiceHidl = android::hardware::cas::V1_2::IMediaCasService;
using ScramblingModeHidl = android::hardware::cas::V1_2::ScramblingMode;
using SessionIntentHidl = android::hardware::cas::V1_2::SessionIntent;
using StatusHidl = android::hardware::cas::V1_2::Status;
using StatusEventHidl = android::hardware::cas::V1_2::StatusEvent;
using aidl::android::hardware::cas::BnCasListener;
using aidl::android::hardware::cas::ICas;
using aidl::android::hardware::cas::IMediaCasService;
using aidl::android::hardware::cas::ScramblingMode;
using aidl::android::hardware::cas::SessionIntent;
using aidl::android::hardware::cas::Status;
using aidl::android::hardware::cas::StatusEvent;
using ndk::ScopedAStatus;

using ::testing::AssertionResult;

using namespace aidl::android::hardware::tv::tuner;

class MediaCasListener : public BnCasListener {
  public:
    virtual ScopedAStatus onEvent(int32_t /*event*/, int32_t /*arg*/,
                                  const std::vector<uint8_t>& /*data*/) override {
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onSessionEvent(const std::vector<uint8_t>& /*sessionId*/,
                                         int32_t /*event*/, int32_t /*arg*/,
                                         const std::vector<uint8_t>& /*data*/) override {
        return ScopedAStatus::ok();
    }

    virtual ScopedAStatus onStatusUpdate(StatusEvent /*event*/, int32_t /*arg*/) override {
        return ScopedAStatus::ok();
    }
};

class MediaCasListenerHidl : public ICasListenerHidl {
  public:
    virtual Return<void> onEvent(int32_t /*event*/, int32_t /*arg*/,
                                 const hidl_vec<uint8_t>& /*data*/) override {
        return Void();
    }

    virtual Return<void> onSessionEvent(const hidl_vec<uint8_t>& /*sessionId*/, int32_t /*event*/,
                                        int32_t /*arg*/,
                                        const hidl_vec<uint8_t>& /*data*/) override {
        return Void();
    }

    virtual Return<void> onStatusUpdate(StatusEventHidl /*event*/, int32_t /*arg*/) override {
        return Void();
    }
};

class DescramblerTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) { mService = tuner; }
    void setCasServiceHidl(sp<IMediaCasServiceHidl> casService) {
        isAidl = false;
        mMediaCasServiceHidl = casService;
    }
    void setCasService(std::shared_ptr<IMediaCasService> casService) {
        isAidl = true;
        mMediaCasService = casService;
    }

    AssertionResult setKeyToken(std::vector<uint8_t>& token);
    AssertionResult openDescrambler(int32_t demuxId);
    AssertionResult addPid(DemuxPid pid, std::shared_ptr<IFilter> optionalSourceFilter);
    AssertionResult removePid(DemuxPid pid, std::shared_ptr<IFilter> optionalSourceFilter);
    AssertionResult closeDescrambler();
    AssertionResult getKeyToken(int32_t caSystemId, std::string& provisonStr,
                                std::vector<uint8_t>& hidlPvtData, std::vector<uint8_t>& token);
    AssertionResult getDemuxPidFromFilterSettings(DemuxFilterType type,
                                                  const DemuxFilterSettings& settings,
                                                  DemuxPid& pid);

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<IDescrambler> mDescrambler;
    android::sp<ICasHidl> mCasHidl;
    android::sp<IMediaCasServiceHidl> mMediaCasServiceHidl;
    android::sp<MediaCasListenerHidl> mCasListenerHidl;
    std::shared_ptr<ICas> mCas;
    std::shared_ptr<IMediaCasService> mMediaCasService;
    std::shared_ptr<MediaCasListener> mCasListener;

  private:
    bool isAidl = false;
    AssertionResult openCasSession(std::vector<uint8_t>& sessionId,
                                   std::vector<uint8_t>& hidlPvtData);
    AssertionResult createCasPlugin(int32_t caSystemId);
};
