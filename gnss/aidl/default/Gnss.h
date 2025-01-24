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

#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/BnGnssConfiguration.h>
#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <aidl/android/hardware/gnss/BnGnssPowerIndication.h>
#include <aidl/android/hardware/gnss/BnGnssPsds.h>
#include "GnssConfiguration.h"
#include "GnssPowerIndication.h"

namespace aidl::android::hardware::gnss {

class Gnss : public BnGnss {
  public:
    ndk::ScopedAStatus setCallback(const std::shared_ptr<IGnssCallback>& callback) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus getExtensionPsds(std::shared_ptr<IGnssPsds>* iGnssPsds) override;
    ndk::ScopedAStatus getExtensionGnssConfiguration(
            std::shared_ptr<IGnssConfiguration>* iGnssConfiguration) override;
    ndk::ScopedAStatus getExtensionGnssPowerIndication(
            std::shared_ptr<IGnssPowerIndication>* iGnssPowerIndication) override;
    ndk::ScopedAStatus getExtensionGnssMeasurement(
            std::shared_ptr<IGnssMeasurementInterface>* iGnssMeasurement) override;

    void reportSvStatus() const;
    std::shared_ptr<GnssConfiguration> mGnssConfiguration;
    std::shared_ptr<GnssPowerIndication> mGnssPowerIndication;

  private:
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
    void reportLocation(const GnssLocation&) const;
    void reportSvStatus() const;
    void reportSvStatus(const std::vector<IGnssCallback::GnssSvInfo>& svInfoList) const;
    std::vector<IGnssCallback::GnssSvInfo> filterBlocklistedSatellites(
            std::vector<IGnssCallback::GnssSvInfo> gnssSvInfoList) const;
    void reportGnssStatusValue(const IGnssCallback::GnssStatusValue gnssStatusValue) const;
    std::unique_ptr<GnssLocation> getLocationFromHW();
    void reportNmea() const;

=======
    void reportLocation(const GnssLocation&) const;
    void reportSvStatus(const std::vector<IGnssCallback::GnssSvInfo>& svInfoList) const;
    std::vector<IGnssCallback::GnssSvInfo> filterBlocklistedSatellites(
            std::vector<IGnssCallback::GnssSvInfo> gnssSvInfoList) const;
    void reportGnssStatusValue(const IGnssCallback::GnssStatusValue gnssStatusValue) const;
    std::unique_ptr<GnssLocation> getLocationFromHW();
    void reportNmea() const;

>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    static std::shared_ptr<IGnssCallback> sGnssCallback;
};

}  // namespace aidl::android::hardware::gnss
