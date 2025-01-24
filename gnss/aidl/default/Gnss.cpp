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

#define LOG_TAG "GnssAidl"

#include "Gnss.h"
#include <log/log.h>
#include "GnssConfiguration.h"
#include "GnssMeasurementInterface.h"
#include "GnssPsds.h"

namespace aidl::android::hardware::gnss {

std::shared_ptr<IGnssCallback> Gnss::sGnssCallback = nullptr;

ndk::ScopedAStatus Gnss::setCallback(const std::shared_ptr<IGnssCallback>& callback) {
    ALOGD("Gnss::setCallback");
    if (callback == nullptr) {
        ALOGE("%s: Null callback ignored", __func__);
        return ndk::ScopedAStatus::fromExceptionCode(STATUS_INVALID_OPERATION);
    }

    sGnssCallback = callback;

<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    int capabilities = (int)(IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST |
                             IGnssCallback::CAPABILITY_SATELLITE_PVT |
                             IGnssCallback::CAPABILITY_CORRELATION_VECTOR);

||||||| BASE
    int capabilities =
            (int)(IGnssCallback::CAPABILITY_MEASUREMENTS | IGnssCallback::CAPABILITY_SCHEDULING |
                  IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST |
                  IGnssCallback::CAPABILITY_SATELLITE_PVT |
                  IGnssCallback::CAPABILITY_CORRELATION_VECTOR |
                  IGnssCallback::CAPABILITY_ANTENNA_INFO);
=======
    int capabilities =
            (int)(IGnssCallback::CAPABILITY_MEASUREMENTS | IGnssCallback::CAPABILITY_SCHEDULING |
                  IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST |
                  IGnssCallback::CAPABILITY_SATELLITE_PVT |
                  IGnssCallback::CAPABILITY_CORRELATION_VECTOR |
                  IGnssCallback::CAPABILITY_ANTENNA_INFO |
                  IGnssCallback::CAPABILITY_ACCUMULATED_DELTA_RANGE);
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    auto status = sGnssCallback->gnssSetCapabilitiesCb(capabilities);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback.gnssSetCapabilities", __func__);
    }

<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    return ndk::ScopedAStatus::ok();
||||||| BASE
    IGnssCallback::GnssSystemInfo systemInfo = {
            .yearOfHw = 2022,
            .name = "Google Mock GNSS Implementation AIDL v2",
    };
    status = sGnssCallback->gnssSetSystemInfoCb(systemInfo);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback.gnssSetSystemInfoCb", __func__);
    }

    return ScopedAStatus::ok();
=======
    IGnssCallback::GnssSystemInfo systemInfo = {
            .yearOfHw = 2022,
            .name = "Google, Cuttlefish, AIDL v3",
    };
    status = sGnssCallback->gnssSetSystemInfoCb(systemInfo);
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback.gnssSetSystemInfoCb", __func__);
    }
    GnssSignalType signalType1 = {
            .constellation = GnssConstellationType::GPS,
            .carrierFrequencyHz = 1.57542e+09,
            .codeType = GnssSignalType::CODE_TYPE_C,
    };
    GnssSignalType signalType2 = {
            .constellation = GnssConstellationType::GLONASS,
            .carrierFrequencyHz = 1.5980625e+09,
            .codeType = GnssSignalType::CODE_TYPE_C,
    };
    status = sGnssCallback->gnssSetSignalTypeCapabilitiesCb(
            std::vector<GnssSignalType>({signalType1, signalType2}));
    if (!status.isOk()) {
        ALOGE("%s: Unable to invoke callback.gnssSetSignalTypeCapabilitiesCb", __func__);
    }
    return ScopedAStatus::ok();
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
}

ndk::ScopedAStatus Gnss::close() {
    ALOGD("Gnss::close");
    sGnssCallback = nullptr;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionPsds(std::shared_ptr<IGnssPsds>* iGnssPsds) {
    ALOGD("Gnss::getExtensionPsds");
    *iGnssPsds = SharedRefBase::make<GnssPsds>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssConfiguration(
        std::shared_ptr<IGnssConfiguration>* iGnssConfiguration) {
    ALOGD("Gnss::getExtensionGnssConfiguration");
    if (mGnssConfiguration == nullptr) {
        mGnssConfiguration = SharedRefBase::make<GnssConfiguration>();
    }
    *iGnssConfiguration = mGnssConfiguration;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssPowerIndication(
        std::shared_ptr<IGnssPowerIndication>* iGnssPowerIndication) {
    ALOGD("Gnss::getExtensionGnssPowerIndication");
    if (mGnssPowerIndication == nullptr) {
        mGnssPowerIndication = SharedRefBase::make<GnssPowerIndication>();
    }

    *iGnssPowerIndication = mGnssPowerIndication;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::getExtensionGnssMeasurement(
        std::shared_ptr<IGnssMeasurementInterface>* iGnssMeasurement) {
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    ALOGD("Gnss::getExtensionGnssMeasurement");
||||||| BASE
    ALOGD("getExtensionGnssMeasurement");
    if (mGnssMeasurementInterface == nullptr) {
        mGnssMeasurementInterface = SharedRefBase::make<GnssMeasurementInterface>();
    }
    *iGnssMeasurement = mGnssMeasurementInterface;
    return ScopedAStatus::ok();
}
=======
    ALOGD("getExtensionGnssMeasurement");
    if (mGnssMeasurementInterface == nullptr) {
        mGnssMeasurementInterface = SharedRefBase::make<GnssMeasurementInterface>();
        mGnssMeasurementInterface->setGnssInterface(static_cast<std::shared_ptr<Gnss>>(this));
    }
    *iGnssMeasurement = mGnssMeasurementInterface;
    return ScopedAStatus::ok();
}
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)

    *iGnssMeasurement = SharedRefBase::make<GnssMeasurementInterface>();
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::gnss
