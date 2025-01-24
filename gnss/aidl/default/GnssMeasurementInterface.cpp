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

#define LOG_TAG "GnssMeasIfaceAidl"

#include "GnssMeasurementInterface.h"
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <log/log.h>
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
#include "DeviceFileReader.h"
#include "GnssRawMeasurementParser.h"
#include "GnssReplayUtils.h"
=======
#include "DeviceFileReader.h"
#include "Gnss.h"
#include "GnssRawMeasurementParser.h"
#include "GnssReplayUtils.h"
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
#include "Utils.h"

namespace aidl::android::hardware::gnss {

using Utils = ::android::hardware::gnss::common::Utils;

std::shared_ptr<IGnssMeasurementCallback> GnssMeasurementInterface::sCallback = nullptr;

GnssMeasurementInterface::GnssMeasurementInterface() : mMinIntervalMillis(1000) {}

GnssMeasurementInterface::~GnssMeasurementInterface() {
    stop();
}

ndk::ScopedAStatus GnssMeasurementInterface::setCallback(
        const std::shared_ptr<IGnssMeasurementCallback>& callback, const bool enableFullTracking,
        const bool enableCorrVecOutputs) {
    ALOGD("setCallback: enableFullTracking: %d enableCorrVecOutputs: %d", (int)enableFullTracking,
          (int)enableCorrVecOutputs);
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    start(enableCorrVecOutputs, enableFullTracking);

    return ndk::ScopedAStatus::ok();
}

<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
ndk::ScopedAStatus GnssMeasurementInterface::setCallbackWithOptions(
        const std::shared_ptr<IGnssMeasurementCallback>& callback, const Options& options) {
    ALOGD("setCallbackWithOptions: fullTracking:%d, corrVec:%d, intervalMs:%d",
          (int)options.enableFullTracking, (int)options.enableCorrVecOutputs, options.intervalMs);
    {
        std::unique_lock<std::mutex> lock(mMutex);
        sCallback = callback;
    }

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    mIntervalMs = std::max(options.intervalMs, 1000);
    start(options.enableCorrVecOutputs);

    return ndk::ScopedAStatus::ok();
}

=======
ndk::ScopedAStatus GnssMeasurementInterface::setCallbackWithOptions(
        const std::shared_ptr<IGnssMeasurementCallback>& callback, const Options& options) {
    ALOGD("setCallbackWithOptions: fullTracking:%d, corrVec:%d, intervalMs:%d",
          (int)options.enableFullTracking, (int)options.enableCorrVecOutputs, options.intervalMs);
    {
        std::unique_lock<std::mutex> lock(mMutex);
        sCallback = callback;
    }

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    mIntervalMs = std::max(options.intervalMs, 1000);
    start(options.enableCorrVecOutputs, options.enableFullTracking);

    return ndk::ScopedAStatus::ok();
}

>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
ndk::ScopedAStatus GnssMeasurementInterface::close() {
    ALOGD("close");
    stop();
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = nullptr;
    return ndk::ScopedAStatus::ok();
}

void GnssMeasurementInterface::start(const bool enableCorrVecOutputs,
                                     const bool enableFullTracking) {
    ALOGD("start");
    mIsActive = true;
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    mThread = std::thread([this, enableCorrVecOutputs]() {
        while (mIsActive == true) {
            auto measurement = Utils::getMockMeasurement(enableCorrVecOutputs);
            this->reportMeasurement(measurement);

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
||||||| BASE
    mThreadBlocker.reset();
    mThread = std::thread([this, enableCorrVecOutputs]() {
        int intervalMs;
        do {
            if (!mIsActive) {
                break;
            }
            std::string rawMeasurementStr = "";
            if (ReplayUtils::hasGnssDeviceFile() &&
                ReplayUtils::isGnssRawMeasurement(
                        rawMeasurementStr =
                                DeviceFileReader::Instance().getGnssRawMeasurementData())) {
                ALOGD("rawMeasurementStr(size: %zu) from device file: %s", rawMeasurementStr.size(),
                      rawMeasurementStr.c_str());
                auto measurement =
                        GnssRawMeasurementParser::getMeasurementFromStrs(rawMeasurementStr);
                if (measurement != nullptr) {
                    this->reportMeasurement(*measurement);
                }
            } else {
                auto measurement = Utils::getMockMeasurement(enableCorrVecOutputs);
                this->reportMeasurement(measurement);
            }
            intervalMs =
                    (mLocationEnabled) ? std::min(mLocationIntervalMs, mIntervalMs) : mIntervalMs;
        } while (mIsActive && mThreadBlocker.wait_for(std::chrono::milliseconds(intervalMs)));
=======
    mThreadBlocker.reset();
    mThread = std::thread([this, enableCorrVecOutputs, enableFullTracking]() {
        int intervalMs;
        do {
            if (!mIsActive) {
                break;
            }
            std::string rawMeasurementStr = "";
            if (ReplayUtils::hasGnssDeviceFile() &&
                ReplayUtils::isGnssRawMeasurement(
                        rawMeasurementStr =
                                DeviceFileReader::Instance().getGnssRawMeasurementData())) {
                ALOGD("rawMeasurementStr(size: %zu) from device file: %s", rawMeasurementStr.size(),
                      rawMeasurementStr.c_str());
                auto measurement =
                        GnssRawMeasurementParser::getMeasurementFromStrs(rawMeasurementStr);
                if (measurement != nullptr) {
                    this->reportMeasurement(*measurement);
                }
            } else {
                auto measurement =
                        Utils::getMockMeasurement(enableCorrVecOutputs, enableFullTracking);
                this->reportMeasurement(measurement);
                if (!mLocationEnabled) {
                    mGnss->reportSvStatus();
                }
            }
            intervalMs =
                    (mLocationEnabled) ? std::min(mLocationIntervalMs, mIntervalMs) : mIntervalMs;
        } while (mIsActive && mThreadBlocker.wait_for(std::chrono::milliseconds(intervalMs)));
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    });
    mThread.detach();
}

void GnssMeasurementInterface::stop() {
    ALOGD("stop");
    mIsActive = false;
}

void GnssMeasurementInterface::reportMeasurement(const GnssData& data) {
    ALOGD("reportMeasurement()");
    std::shared_ptr<IGnssMeasurementCallback> callbackCopy;
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (sCallback == nullptr) {
            ALOGE("%s: GnssMeasurement::sCallback is null.", __func__);
            return;
        }
        callbackCopy = sCallback;
    }
    callbackCopy->gnssMeasurementCb(data);
}

<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
void GnssMeasurementInterface::setLocationInterval(const int intervalMs) {
    mLocationIntervalMs = intervalMs;
}

void GnssMeasurementInterface::setLocationEnabled(const bool enabled) {
    mLocationEnabled = enabled;
}

void GnssMeasurementInterface::waitForStoppingThreads() {
    for (auto& future : mFutures) {
        ALOGD("Stopping previous thread.");
        future.wait();
        ALOGD("Done stopping thread.");
    }
    mFutures.clear();
}

=======
void GnssMeasurementInterface::setLocationInterval(const int intervalMs) {
    mLocationIntervalMs = intervalMs;
}

void GnssMeasurementInterface::setLocationEnabled(const bool enabled) {
    mLocationEnabled = enabled;
}

void GnssMeasurementInterface::setGnssInterface(const std::shared_ptr<Gnss>& gnss) {
    mGnss = gnss;
}

void GnssMeasurementInterface::waitForStoppingThreads() {
    for (auto& future : mFutures) {
        ALOGD("Stopping previous thread.");
        future.wait();
        ALOGD("Done stopping thread.");
    }
    mFutures.clear();
}

>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
}  // namespace aidl::android::hardware::gnss
