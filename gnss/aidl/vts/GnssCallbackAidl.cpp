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

#include "GnssCallbackAidl.h"
#include <log/log.h>
#include <utils/SystemClock.h>

<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
android::binder::Status GnssCallbackAidl::gnssSetCapabilitiesCb(const int capabilities) {
    ALOGI("Capabilities received %d", capabilities);
||||||| BASE
using android::binder::Status;
using android::hardware::gnss::GnssLocation;
using GnssSvInfo = android::hardware::gnss::IGnssCallback::GnssSvInfo;
using GnssSystemInfo = android::hardware::gnss::IGnssCallback::GnssSystemInfo;

Status GnssCallbackAidl::gnssSetCapabilitiesCb(const int capabilities) {
    ALOGI("Capabilities received %#08x", capabilities);
=======
using android::binder::Status;
using android::hardware::gnss::GnssLocation;
using GnssSvInfo = android::hardware::gnss::IGnssCallback::GnssSvInfo;
using GnssSystemInfo = android::hardware::gnss::IGnssCallback::GnssSystemInfo;
using GnssSignalType = android::hardware::gnss::GnssSignalType;

Status GnssCallbackAidl::gnssSetCapabilitiesCb(const int capabilities) {
    ALOGI("Capabilities received %#08x", capabilities);
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    capabilities_cbq_.store(capabilities);
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    return android::binder::Status::ok();
||||||| BASE
    return Status::ok();
}

Status GnssCallbackAidl::gnssStatusCb(const GnssStatusValue /* status */) {
    ALOGI("gnssStatusCb");
    return Status::ok();
}

Status GnssCallbackAidl::gnssSvStatusCb(const std::vector<GnssSvInfo>& svInfoList) {
    ALOGI("gnssSvStatusCb. Size = %d", (int)svInfoList.size());
    sv_info_list_cbq_.store(svInfoList);
    return Status::ok();
}

Status GnssCallbackAidl::gnssLocationCb(const GnssLocation& location) {
    ALOGI("Location received");
    location_cbq_.store(location);
    return Status::ok();
}

Status GnssCallbackAidl::gnssNmeaCb(const int64_t timestamp, const std::string& nmea) {
    nmea_cbq_.store(std::make_pair(timestamp, nmea));
    return Status::ok();
}

Status GnssCallbackAidl::gnssAcquireWakelockCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssReleaseWakelockCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssSetSystemInfoCb(const GnssSystemInfo& info) {
    ALOGI("gnssSetSystemInfoCb, year=%d, name=%s", info.yearOfHw, info.name.c_str());
    info_cbq_.store(info);
    return Status::ok();
}

Status GnssCallbackAidl::gnssRequestTimeCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssRequestLocationCb(const bool /* independentFromGnss */,
                                               const bool /* isUserEmergency */) {
    return Status::ok();
=======
    return Status::ok();
}

Status GnssCallbackAidl::gnssSetSignalTypeCapabilitiesCb(
        const std::vector<GnssSignalType>& signalTypes) {
    ALOGI("SignalTypeCapabilities received");
    std::ostringstream ss;
    for (auto& signalType : signalTypes) {
        ss << "[constellation=" << (int)signalType.constellation
           << ", carrierFrequencyHz=" << signalType.carrierFrequencyHz
           << ", codeType=" << signalType.codeType << "], ";
    }
    ALOGI("%s", ss.str().c_str());
    signal_type_capabilities_cbq_.store(signalTypes);
    return Status::ok();
}

Status GnssCallbackAidl::gnssStatusCb(const GnssStatusValue /* status */) {
    ALOGI("gnssStatusCb");
    return Status::ok();
}

Status GnssCallbackAidl::gnssSvStatusCb(const std::vector<GnssSvInfo>& svInfoList) {
    ALOGI("gnssSvStatusCb. Size = %d", (int)svInfoList.size());
    sv_info_list_cbq_.store(svInfoList);
    sv_info_list_timestamps_millis_cbq_.store(::android::elapsedRealtime());
    return Status::ok();
}

Status GnssCallbackAidl::gnssLocationCb(const GnssLocation& location) {
    ALOGI("Location received");
    location_cbq_.store(location);
    return Status::ok();
}

Status GnssCallbackAidl::gnssNmeaCb(const int64_t timestamp, const std::string& nmea) {
    nmea_cbq_.store(std::make_pair(timestamp, nmea));
    return Status::ok();
}

Status GnssCallbackAidl::gnssAcquireWakelockCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssReleaseWakelockCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssSetSystemInfoCb(const GnssSystemInfo& info) {
    ALOGI("gnssSetSystemInfoCb, year=%d, name=%s", info.yearOfHw, info.name.c_str());
    info_cbq_.store(info);
    return Status::ok();
}

Status GnssCallbackAidl::gnssRequestTimeCb() {
    return Status::ok();
}

Status GnssCallbackAidl::gnssRequestLocationCb(const bool /* independentFromGnss */,
                                               const bool /* isUserEmergency */) {
    return Status::ok();
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
}
