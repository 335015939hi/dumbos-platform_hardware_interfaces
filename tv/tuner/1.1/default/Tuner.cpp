/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.tuner@1.1-Tuner"

#include "Tuner.h"
#include <utils/Log.h>
#include "Demux.h"
#include "Descrambler.h"
#include "Frontend.h"
#include "Lnb.h"

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Tuner::Tuner() {
    // Static Frontends array to maintain local frontends information
    // Array index matches their FrontendId in the default impl
    mFrontendSize = 1;
    mFrontends[0] = new Frontend(FrontendType::DVBT, 0, this);

    FrontendInfo::FrontendCapabilities caps;
    vector<FrontendStatusType> statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.dvbtCaps(FrontendDvbtCapabilities());
    mFrontendCaps[0] = caps;
    statusCaps = {
            FrontendStatusType::EWBS,
            FrontendStatusType::PLP_ID,
            FrontendStatusType::HIERARCHY,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::T2_SYSTEM_ID),
    };
    mFrontendStatusCaps[0] = statusCaps;

    statusCaps = {
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::INTERLEAVINGS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE),
    };
    mFrontendStatusCaps[1] = statusCaps;

    //mLnbs.resize(2);
    //mLnbs[0] = new Lnb(0);
    //mLnbs[1] = new Lnb(1);	
}

Tuner::~Tuner() {}

Return<void> Tuner::getFrontendIds(getFrontendIds_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

	ALOGD("%s> Def-tunerhal Tuner getFrontendIds ",  __FUNCTION__);   // Anbu - for testing
	_hidl_cb(Result::SUCCESS, 1);
	return Void();

    vector<FrontendId> frontendIds;
    frontendIds.resize(mFrontendSize);
    for (int i = 0; i < mFrontendSize; i++) {
        frontendIds[i] = mFrontends[i]->getFrontendId();
    }

    _hidl_cb(Result::SUCCESS, frontendIds);
    return Void();
}

Return<void> Tuner::openFrontendById(uint32_t frontendId, openFrontendById_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (frontendId >= mFrontendSize || frontendId < 0) {
        ALOGW("[   WARN   ] Frontend with id %d isn't available", frontendId);
        _hidl_cb(Result::UNAVAILABLE, nullptr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mFrontends[frontendId]);
    return Void();
}

Return<void> Tuner::openDemux(openDemux_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint32_t demuxId = mLastUsedId + 1;
    mLastUsedId += 1;
    sp<Demux> demux = new Demux(demuxId, this);
    mDemuxes[demuxId] = demux;

    _hidl_cb(Result::SUCCESS, demuxId, demux);
    return Void();
}

Return<void> Tuner::getDemuxCaps(getDemuxCaps_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    DemuxCapabilities caps;

    // IP filter can be an MMTP filter's data source.
    caps.linkCaps = {0x00, 0x00, 0x02, 0x00, 0x00};
    // Support time filter testing
    caps.bTimeFilter = true;
    _hidl_cb(Result::SUCCESS, caps);
    return Void();
}

Return<void> Tuner::openDescrambler(openDescrambler_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<V1_0::IDescrambler> descrambler = new Descrambler();

    _hidl_cb(Result::SUCCESS, descrambler);
    return Void();
}

Return<void> Tuner::getFrontendInfo(FrontendId frontendId, getFrontendInfo_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    FrontendInfo info;
    if (frontendId >= mFrontendSize) {
        _hidl_cb(Result::INVALID_ARGUMENT, info);
        return Void();
    }

    // assign randomly selected values for testing.
    info = {
            .type = mFrontends[frontendId]->getFrontendType(),
            .minFrequency = 139,
            .maxFrequency = 1139,
            .minSymbolRate = 45,
            .maxSymbolRate = 1145,
            .acquireRange = 30,
            .exclusiveGroupId = 57,
            .statusCaps = mFrontendStatusCaps[frontendId],
            .frontendCaps = mFrontendCaps[frontendId],
    };

    _hidl_cb(Result::SUCCESS, info);
    return Void();
}

Return<void> Tuner::getLnbIds(getLnbIds_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<V1_0::LnbId> lnbIds;
    lnbIds.resize(mLnbs.size());
    for (int i = 0; i < lnbIds.size(); i++) {
        lnbIds[i] = mLnbs[i]->getId();
    }

    _hidl_cb(Result::SUCCESS, lnbIds);
    return Void();
}

Return<void> Tuner::openLnbById(V1_0::LnbId lnbId, openLnbById_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (lnbId >= mLnbs.size()) {
        _hidl_cb(Result::INVALID_ARGUMENT, nullptr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mLnbs[lnbId]);
    return Void();
}

sp<Frontend> Tuner::getFrontendById(uint32_t frontendId) {
    ALOGV("%s", __FUNCTION__);

    return mFrontends[frontendId];
}

Return<void> Tuner::openLnbByName(const hidl_string& /*lnbName*/, openLnbByName_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<V1_0::ILnb> lnb = new Lnb();

    _hidl_cb(Result::SUCCESS, 1234, lnb);
    return Void();
}

Return<void> Tuner::getFrontendDtmbCapabilities(uint32_t frontendId,
                                                getFrontendDtmbCapabilities_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (mFrontends[frontendId] != nullptr &&
        (mFrontends[frontendId]->getFrontendType() ==
         static_cast<V1_0::FrontendType>(V1_1::FrontendType::DTMB))) {
        _hidl_cb(Result::SUCCESS, mDtmbCaps);
    } else {
        _hidl_cb(Result::UNAVAILABLE, mDtmbCaps);
    }
    return Void();
}

void Tuner::setFrontendAsDemuxSource(uint32_t frontendId, uint32_t demuxId) {
	return; //Anbu - for testing
    mFrontendToDemux[frontendId] = demuxId;
    if (mFrontends[frontendId] != nullptr && mFrontends[frontendId]->isLocked()) {
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

void Tuner::removeDemux(uint32_t demuxId) {
    map<uint32_t, uint32_t>::iterator it;
    for (it = mFrontendToDemux.begin(); it != mFrontendToDemux.end(); it++) {
        if (it->second == demuxId) {
            it = mFrontendToDemux.erase(it);
            break;
        }
    }
    mDemuxes.erase(demuxId);
}

void Tuner::removeFrontend(uint32_t frontendId) {
    mFrontendToDemux.erase(frontendId);
}

void Tuner::frontendStopTune(uint32_t frontendId) {
    map<uint32_t, uint32_t>::iterator it = mFrontendToDemux.find(frontendId);
    uint32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->stopFrontendInput();
    }
}

void Tuner::frontendStartTune(uint32_t frontendId) {
	return;  // Anbu -for testing
    map<uint32_t, uint32_t>::iterator it = mFrontendToDemux.find(frontendId);
    uint32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
