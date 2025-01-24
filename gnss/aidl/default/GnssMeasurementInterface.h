/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <aidl/android/hardware/gnss/BnGnssMeasurementCallback.h>
#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <atomic>
#include <mutex>
#include <thread>

namespace aidl::android::hardware::gnss {
class Gnss;

struct GnssMeasurementInterface : public BnGnssMeasurementInterface {
  public:
    GnssMeasurementInterface();
    ~GnssMeasurementInterface();
    ndk::ScopedAStatus setCallback(const std::shared_ptr<IGnssMeasurementCallback>& callback,
                                   const bool enableFullTracking,
                                   const bool enableCorrVecOutputs) override;
    ndk::ScopedAStatus close() override;
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
    ndk::ScopedAStatus setCallbackWithOptions(
            const std::shared_ptr<IGnssMeasurementCallback>& callback,
            const Options& options) override;
    void setLocationInterval(const int intervalMs);
    void setLocationEnabled(const bool enabled);
=======
    ndk::ScopedAStatus setCallbackWithOptions(
            const std::shared_ptr<IGnssMeasurementCallback>& callback,
            const Options& options) override;
    void setLocationInterval(const int intervalMs);
    void setLocationEnabled(const bool enabled);
    void setGnssInterface(const std::shared_ptr<Gnss>& gnss);
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)

  private:
    void start(const bool enableCorrVecOutputs, const bool enableFullTracking);
    void stop();
    void reportMeasurement(const GnssData&);

    std::atomic<long> mMinIntervalMillis;
    std::atomic<bool> mIsActive;
    std::thread mThread;

    // Guarded by mMutex
    static std::shared_ptr<IGnssMeasurementCallback> sCallback;

    // Synchronization lock for sCallback
    mutable std::mutex mMutex;

    std::shared_ptr<Gnss> mGnss;
};

}  // namespace aidl::android::hardware::gnss
