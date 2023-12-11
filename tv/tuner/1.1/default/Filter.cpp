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

#define LOG_TAG "android.hardware.tv.tuner@1.1-Filter"

#include "Filter.h"
#include <BufferAllocator/BufferAllocator.h>
#include <utils/Log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <utility>
#include <memory>
#include <algorithm>

#include "FileTuner/TsPlayPump/tsEnumUtils.h"
#include "FileTuner/TsPlayPump/tsTSPacket.h"
#include "FileTuner/TsPlayPump/tsTS.h"
#include "FileTuner/TsPlayPump/tsSectionDemux.h"
#include "FileTuner/TsPlayPump/tsBinaryTable.h"
#include "FileTuner/TsPlayPump/tsAbstractLongTable.h"
#include "FileTuner/TsPlayPump/tsDuckContext.h"
#include "FileTuner/TsPlayPump/tsSDT.h"

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

#define WAIT_TIMEOUT 3000000000
#define SDT_PID 0x11
#define PMT_TABLE_ID 2
#define IS_32BIT (sizeof(long) == 4 ? true : false)

Filter::Filter() {}

Filter::Filter(DemuxFilterType type, uint64_t filterId, uint32_t bufferSize,
               const sp<IFilterCallback>& cb, sp<Demux> demux) {
    mType = type;
    mFilterId = filterId;
    mBufferSize = bufferSize;
    mDemux = demux;

    mLastVersion = -1;
    ts::DuckContext* pesDuckContext = new ts::DuckContext();
    mPes_demux = new ts::PESDemux(*pesDuckContext, this);
    ts::DuckContext* duck = new ts::DuckContext();
    mSectionDemux = new ts::SectionDemux(*duck, this, this);
    mDuckContext = new ts::DuckContext();

    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            if (mType.subType.tsFilterType() == DemuxTsFilterType::AUDIO ||
                mType.subType.tsFilterType() == DemuxTsFilterType::VIDEO) {
                mIsMediaFilter = true;
            }
            if (mType.subType.tsFilterType() == DemuxTsFilterType::PCR) {
                mIsPcrFilter = true;
            }
            if (mType.subType.tsFilterType() == DemuxTsFilterType::RECORD) {
                mIsRecordFilter = true;
            }
            break;
        case DemuxFilterMainType::MMTP:
            if (mType.subType.mmtpFilterType() == DemuxMmtpFilterType::AUDIO ||
                mType.subType.mmtpFilterType() == DemuxMmtpFilterType::VIDEO) {
                mIsMediaFilter = true;
            }
            if (mType.subType.mmtpFilterType() == DemuxMmtpFilterType::RECORD) {
                mIsRecordFilter = true;
            }
            break;
        case DemuxFilterMainType::IP:
            break;
        case DemuxFilterMainType::TLV:
            break;
        case DemuxFilterMainType::ALP:
            break;
        default:
            break;
    }

    sp<V1_1::IFilterCallback> filterCallback_v1_1 = V1_1::IFilterCallback::castFrom(cb);
    if (filterCallback_v1_1 != NULL) {
        mCallback_1_1 = filterCallback_v1_1;
    }
    mCallback = cb;
}

Filter::~Filter() {
    mFilterStarted = false;
    mLastVersion = -1;
    delete mPes_demux;
    delete mSectionDemux;
    delete mDuckContext;
}

Return<void> Filter::getId64Bit(getId64Bit_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    _hidl_cb(Result::SUCCESS, mFilterId);
    return Void();
}

Return<void> Filter::getId(getId_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    _hidl_cb(Result::SUCCESS, static_cast<uint32_t>(mFilterId));
    return Void();
}

Return<Result> Filter::setDataSource(const sp<V1_0::IFilter>& filter) {
    ALOGV("%s", __FUNCTION__);

    mDataSource = filter;
    mIsDataSourceDemux = false;

    return Result::SUCCESS;
}

Return<void> Filter::getQueueDesc(getQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    mIsUsingFMQ = mIsRecordFilter ? false : true;

    _hidl_cb(Result::SUCCESS, *mFilterMQ->getDesc());
    return Void();
}

Return<Result> Filter::configure(const DemuxFilterSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    if (mConfigured) {
        mStartId++;
    }

    if (settings.getDiscriminator() == DemuxFilterSettings::hidl_discriminator::ts) {
        if (settings.ts().filterSettings.getDiscriminator() ==
                DemuxTsFilterSettings::FilterSettings::hidl_discriminator::section) {
            DemuxFilterSectionSettings sectionSettings = settings.ts().filterSettings.section();
            if (sectionSettings.condition.getDiscriminator() ==
                    DemuxFilterSectionSettings::Condition::hidl_discriminator::sectionBits) {
                int fSize = sectionSettings.condition.sectionBits().filter.size();
                if (fSize >= 6) {
                    uint8_t version = sectionSettings.condition.sectionBits().filter[5];
                    uint8_t mask = sectionSettings.condition.sectionBits().mask[5];
                    if (mask != 0) {
                        version = version >> 1;
                        mLastVersion = version;
                    }
                }
            }
        }
    }
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            mTpid = settings.ts().tpid;
            mPes_demux->addPID(mTpid);
            break;
        case DemuxFilterMainType::MMTP:
            break;
        case DemuxFilterMainType::IP:
            break;
        case DemuxFilterMainType::TLV:
            break;
        case DemuxFilterMainType::ALP:
            break;
        default:
            break;
    }

    mConfigured = true;
    return Result::SUCCESS;
}

Return<Result> Filter::start() {
    ALOGV("%s", __FUNCTION__);
    mFilterStarted = true;
    // For testing purpose.
    if (mCallback_1_1 != NULL) {
        DemuxFilterEvent emptyFilterEvent;
        mCallback_1_1->onFilterEvent_1_1(emptyFilterEvent, createRestartEvent());
    }
    if (mIsMediaFilter) {
        return startFilterLoop();
    } else {
        return Result::SUCCESS;
    }
}

Return<Result> Filter::stop() {
    if (mIsMediaFilter) {
        mFilterThreadRunning = false;
        std::lock_guard<std::mutex> lock(mFilterThreadLock);
    }

    mFilterStarted = false;
    return Result::SUCCESS;
}

Return<Result> Filter::flush() {
    ALOGV("%s", __FUNCTION__);

    // temp implementation to flush the FMQ
    int size = mFilterMQ->availableToRead();
    char* buffer = new char[size];
    mFilterMQ->read((unsigned char*)&buffer[0], size);
    delete[] buffer;
    mFilterStatus = DemuxFilterStatus::DATA_READY;
    return Result::SUCCESS;
}

Return<Result> Filter::releaseAvHandle(const hidl_handle& avMemory, uint64_t avDataId) {
    if (mSharedAvMemHandle != NULL && avMemory != NULL &&
        (mSharedAvMemHandle.getNativeHandle()->numFds > 0) &&
        (avMemory.getNativeHandle()->numFds > 0) &&
        (sameFile(avMemory.getNativeHandle()->data[0],
                  mSharedAvMemHandle.getNativeHandle()->data[0]))) {
        freeSharedAvHandle();
        return Result::SUCCESS;
    }

    if (mDataId2Avfd.find(avDataId) == mDataId2Avfd.end()) {
        return Result::INVALID_ARGUMENT;
    }

    ::close(mDataId2Avfd[avDataId]);
    return Result::SUCCESS;
}

Return<Result> Filter::close() {
    ALOGV("%s", __FUNCTION__);
    mFilterStarted = false;
    return mDemux->removeFilter(mFilterId);
}

Return<Result> Filter::configureIpCid(uint32_t ipCid) {
    ALOGV("%s", __FUNCTION__);

    if (mType.mainType != DemuxFilterMainType::IP) {
        return Result::INVALID_STATE;
    }

    mCid = ipCid;
    return Result::SUCCESS;
}

Return<void> Filter::getAvSharedHandle(getAvSharedHandle_cb _hidl_cb) {
    ALOGD("%s", __FUNCTION__);

    if (!mIsMediaFilter) {
        _hidl_cb(Result::INVALID_STATE, NULL, BUFFER_SIZE_16M);
        return Void();
    }

    if (mSharedAvMemHandle.getNativeHandle() != nullptr) {
        _hidl_cb(Result::SUCCESS, mSharedAvMemHandle, BUFFER_SIZE_16M);
        mUsingSharedAvMem = true;
        return Void();
    }
    int av_fd = createAvIonFd(BUFFER_SIZE_16M);
    if (av_fd == -1) {
        _hidl_cb(Result::UNKNOWN_ERROR, NULL, 0);
        return Void();
    }

    native_handle_t* nativeHandle = createNativeHandle(av_fd);
    if (nativeHandle == NULL) {
        ::close(av_fd);
        _hidl_cb(Result::UNKNOWN_ERROR, NULL, 0);
        return Void();
    }
    mSharedAvMemHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    ::close(av_fd);

    _hidl_cb(Result::SUCCESS, mSharedAvMemHandle, BUFFER_SIZE_16M);
    mUsingSharedAvMem = true;
    return Void();
}

Return<Result> Filter::configureAvStreamType(const V1_1::AvStreamType& avStreamType) {
    ALOGV("%s", __FUNCTION__);

    if (!mIsMediaFilter) {
        return Result::UNAVAILABLE;
    }

    switch (avStreamType.getDiscriminator()) {
        case V1_1::AvStreamType::hidl_discriminator::audio:
            mAudioStreamType = static_cast<uint32_t>(avStreamType.audio());
            break;
        case V1_1::AvStreamType::hidl_discriminator::video:
            mVideoStreamType = static_cast<uint32_t>(avStreamType.video());
            break;
        default:
            break;
    }
    return Result::SUCCESS;
}

Return<Result> Filter::configureMonitorEvent(uint32_t monitorEventTypes) {
    ALOGV("%s", __FUNCTION__);

    DemuxFilterEvent emptyFilterEvent;
    V1_1::DemuxFilterMonitorEvent monitorEvent;
    V1_1::DemuxFilterEventExt eventExt;
    uint32_t newScramblingStatus =
            monitorEventTypes & V1_1::DemuxFilterMonitorEventType::SCRAMBLING_STATUS;
    uint32_t newIpCid = monitorEventTypes & V1_1::DemuxFilterMonitorEventType::IP_CID_CHANGE;

    // if scrambling status monitoring flipped, record the new state and send msg on enabling
    if (newScramblingStatus ^ mScramblingStatusMonitored) {
        mScramblingStatusMonitored = newScramblingStatus;
        if (mScramblingStatusMonitored) {
            if (mCallback_1_1 != nullptr) {
                // Assuming current status is always NOT_SCRAMBLED
                monitorEvent.scramblingStatus(V1_1::ScramblingStatus::NOT_SCRAMBLED);
                eventExt.events.resize(1);
                eventExt.events[0].monitorEvent(monitorEvent);
                mCallback_1_1->onFilterEvent_1_1(emptyFilterEvent, eventExt);
            } else {
                return Result::INVALID_STATE;
            }
        }
    }

    // if ip cid monitoring flipped, record the new state and send msg on enabling
    if (newIpCid ^ mIpCidMonitored) {
        mIpCidMonitored = newIpCid;
        if (mIpCidMonitored) {
            if (mCallback_1_1 != nullptr) {
                // Return random cid
                monitorEvent.cid(1);
                eventExt.events.resize(1);
                eventExt.events[0].monitorEvent(monitorEvent);
                mCallback_1_1->onFilterEvent_1_1(emptyFilterEvent, eventExt);
            } else {
                return Result::INVALID_STATE;
            }
        }
    }

    return Result::SUCCESS;
}

bool Filter::createFilterMQ() {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(mBufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("[Filter] Failed to create FMQ of filter with id: %" PRIu64, mFilterId);
        return false;
    }

    mFilterMQ = std::move(tmpFilterMQ);

    if (EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterEventFlag) != OK) {
        return false;
    }

    return true;
}

Result Filter::startFilterLoop() {
    pthread_create(&mFilterThread, NULL, __threadLoopFilter, this);
    pthread_setname_np(mFilterThread, "filter_waiting_loop");
    return Result::SUCCESS;
}

void* Filter::__threadLoopFilter(void* user) {
    Filter* const self = static_cast<Filter*>(user);
    self->filterThreadLoop();
    return 0;
}

void Filter::filterThreadLoop() {
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    mFilterThreadRunning = true;

    while (mFilterThreadRunning) {
        bool empty;
        {
            std::lock_guard<std::mutex> lock(mFilterOutputLock);
            empty = avDeque.empty();
        }

        if (empty) {
            usleep(10 * 1000);
            continue;
        }

        std::lock_guard<std::mutex> lock(mFilterOutputLock);
        vector<uint8_t> vec = avDeque.back();

        ts::TSPacket *pkt = new ts::TSPacket();
        pkt->copyFrom(reinterpret_cast<void*> (vec.data()));
        if (pkt->hasPTS()) {
            mPts = pkt->getPTS();
        }

        mPes_demux->feedPacket(*pkt);
        avDeque.pop_back();
        delete pkt;
    }
}

void Filter::freeAvHandle() {
    if (!mIsMediaFilter) {
        return;
    }
    for (int i = 0; i < mFilterEvent.events.size(); i++) {
        ::close(mFilterEvent.events[i].media().avMemory.getNativeHandle()->data[0]);
        native_handle_delete(const_cast<native_handle_t*>(
                mFilterEvent.events[i].media().avMemory.getNativeHandle()));
    }
}

void Filter::freeSharedAvHandle() {
    if (!mIsMediaFilter) {
        return;
    }
    ::close(mSharedAvMemHandle.getNativeHandle()->data[0]);
    native_handle_delete(const_cast<native_handle_t*>(mSharedAvMemHandle.getNativeHandle()));
}

void Filter::maySendFilterStatusCallback() {
    if (!mIsUsingFMQ) {
        return;
    }
    std::lock_guard<std::mutex> lock(mFilterStatusLock);
    int availableToRead = mFilterMQ->availableToRead();
    int availableToWrite = mFilterMQ->availableToWrite();
    int fmqSize = mFilterMQ->getQuantumCount();

    DemuxFilterStatus newStatus = checkFilterStatusChange(
            availableToWrite, availableToRead, ceil(fmqSize * 0.75), ceil(fmqSize * 0.25));
    if (mFilterStatus != newStatus) {
        if (mCallback != nullptr) {
            mCallback->onFilterStatus(newStatus);
        } else if (mCallback_1_1 != nullptr) {
            mCallback_1_1->onFilterStatus(newStatus);
        }
        mFilterStatus = newStatus;
    }
}

DemuxFilterStatus Filter::checkFilterStatusChange(uint32_t availableToWrite,
                                                  uint32_t availableToRead, uint32_t highThreshold,
                                                  uint32_t lowThreshold) {
    if (availableToWrite == 0) {
        return DemuxFilterStatus::OVERFLOW;
    } else if (availableToRead > highThreshold) {
        return DemuxFilterStatus::HIGH_WATER;
    } else if (availableToRead < lowThreshold) {
        return DemuxFilterStatus::LOW_WATER;
    }
    return mFilterStatus;
}

uint16_t Filter::getTpid() {
    return mTpid;
}

void Filter::updateFilterOutput(vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);

    if (mFilterStarted == true) {
        if (mIsMediaFilter) {
            avDeque.push_front(data);
        } else {
            mFilterOutput.insert(mFilterOutput.end(), data.begin(), data.end());
        }
        startFilterHandler();
    }
}

void Filter::updatePts(uint64_t pts) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    mPts = pts;
}

void Filter::updatePcr(uint64_t pcr) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    mPcr = pcr;
}

uint64_t Filter::getFilterId() {
    return mFilterId;
}

uint64_t Filter::getPcr() {
    return mPcr;
}

uint64_t Filter::getPts() {
    return mPts;
}

void Filter::updateRecordOutput(vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(mRecordFilterOutputLock);
    mRecordFilterOutput.insert(mRecordFilterOutput.end(), data.begin(), data.end());
}

Result Filter::startFilterHandler() {
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            switch (mType.subType.tsFilterType()) {
                case DemuxTsFilterType::UNDEFINED:
                    break;
                case DemuxTsFilterType::SECTION:
                    startSectionFilterHandler();
                    break;
                case DemuxTsFilterType::RECORD:
                    startRecordFilterHandler();
                    break;
                case DemuxTsFilterType::PES:
                    startPesFilterHandler();
                    break;
                case DemuxTsFilterType::TS:
                    startTsFilterHandler();
                    break;
                case DemuxTsFilterType::AUDIO:
                case DemuxTsFilterType::VIDEO:
                    break;
                case DemuxTsFilterType::PCR:
                    startPcrFilterHandler();
                    break;
                case DemuxTsFilterType::TEMI:
                    startTemiFilterHandler();
                    break;
                default:
                    break;
            }
            break;
        case DemuxFilterMainType::MMTP:
            /*mmtpSettings*/
            break;
        case DemuxFilterMainType::IP:
            /*ipSettings*/
            break;
        case DemuxFilterMainType::TLV:
            /*tlvSettings*/
            break;
        case DemuxFilterMainType::ALP:
            /*alpSettings*/
            break;
        default:
            break;
    }
    return Result::SUCCESS;
}

Result Filter::startSectionFilterHandler() {
    if (mFilterOutput.empty()) {
        return Result::SUCCESS;
    }
    if (!writeSectionsAndCreateEvent(mFilterOutput)) {
        ALOGD("[Filter] filter %" PRIu64 " fails to write into FMQ. Ending thread", mFilterId);
        return Result::UNKNOWN_ERROR;
    }
    mFilterOutput.clear();
    return Result::SUCCESS;
}

Result Filter::startPesFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (mFilterOutput.empty()) {
        return Result::SUCCESS;
    }

    for (int i = 0; i < mFilterOutput.size(); i += 188) {
        if (mPesSizeLeft == 0) {
            uint32_t prefix = (mFilterOutput[i + 4] << 16) | (mFilterOutput[i + 5] << 8) |
                              mFilterOutput[i + 6];
            if (DEBUG_FILTER) {
                ALOGD("[Filter] prefix %d", prefix);
            }
            if (prefix == 0x000001) {
                mPesSizeLeft = (mFilterOutput[i + 8] << 8) | mFilterOutput[i + 9];
                mPesSizeLeft += 6;
                if (DEBUG_FILTER) {
                    ALOGD("[Filter] pes data length %d", mPesSizeLeft);
                }
            } else {
                continue;
            }
        }

        int endPoint = min(184, mPesSizeLeft);
        // append data and check size
        vector<uint8_t>::const_iterator first = mFilterOutput.begin() + i + 4;
        vector<uint8_t>::const_iterator last = mFilterOutput.begin() + i + 4 + endPoint;
        mPesOutput.insert(mPesOutput.end(), first, last);
        // size does not match then continue
        mPesSizeLeft -= endPoint;
        if (DEBUG_FILTER) {
            ALOGD("[Filter] pes data left %d", mPesSizeLeft);
        }
        if (mPesSizeLeft > 0) {
            continue;
        }
        // size match then create event
        if (!writeDataToFilterMQ(mPesOutput)) {
            ALOGD("[Filter] pes data write failed");
            mFilterOutput.clear();
            return Result::INVALID_STATE;
        }
        maySendFilterStatusCallback();
        DemuxFilterPesEvent pesEvent;
        pesEvent = {
                // temp dump meta data
                .streamId = mPesOutput[3],
                .dataLength = static_cast<uint16_t>(mPesOutput.size()),
        };
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled pes data length %d", pesEvent.dataLength);
        }

        int size = mFilterEvent.events.size();
        mFilterEvent.events.resize(size + 1);
        mFilterEvent.events[size].pes(pesEvent);
        mPesOutput.clear();
    }

    mFilterOutput.clear();
    return Result::SUCCESS;
}

Result Filter::startTsFilterHandler() {
    return Result::SUCCESS;
}

void Filter::handlePESPacket(ts::PESDemux& demux, const ts::PESPacket& packet) {
    vector<uint8_t> data(packet.payload(), packet.payload() + packet.payloadSize());
    createMediaFilterEventWithIon(data);
    return;
}

void Filter::handleVideoStartCode(ts::PESDemux& demux, const ts::PESPacket& packet,
                                  uint8_t start_code, size_t offset, size_t size) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleNewMPEG2VideoAttributes(ts::PESDemux& demux, const ts::PESPacket& packet,
                                           const ts::MPEG2VideoAttributes& attr) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleAccessUnit(ts::PESDemux& demux, const ts::PESPacket& packet,
                              uint8_t nal_unit_type, size_t offset, size_t size) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleSEI(ts::PESDemux& demux, const ts::PESPacket& packet,
                       uint32_t sei_type, size_t offset, size_t size) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleNewAVCAttributes(ts::PESDemux& demux, const ts::PESPacket& packet,
                                    const ts::AVCAttributes& attr) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleNewHEVCAttributes(ts::PESDemux& demux, const ts::PESPacket& packet,
                                     const ts::HEVCAttributes& attr) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleIntraImage(ts::PESDemux& demux, const ts::PESPacket& packet, size_t offset) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleNewMPEG2AudioAttributes(ts::PESDemux& demux, const ts::PESPacket& packet,
                                           const ts::MPEG2AudioAttributes& attr) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

void Filter::handleNewAC3Attributes(ts::PESDemux& demux, const ts::PESPacket& packet,
                                    const ts::AC3Attributes& attr) {
    ALOGD("%s %d", __FUNCTION__, getTpid());
}

Result Filter::createMediaFilterEventWithIon(vector<uint8_t> output) {
    if (mUsingSharedAvMem) {
        if (mSharedAvMemHandle.getNativeHandle() == nullptr) {
            return Result::UNKNOWN_ERROR;
        }
        return createShareMemMediaEvents(output);
    }
    return createIndependentMediaEvents(output);
}

Result Filter::startRecordFilterHandler() {
    if (mFilterOutput.empty()) {
        ALOGD("[Filter] %s Empty Data" , __FUNCTION__);
        return Result::SUCCESS;
    }

    if (mDvr == nullptr || !mDvr->writeRecordFMQ(mFilterOutput)) {
        ALOGD("[Filter] dvr fails to write into record FMQ.");
        return Result::UNKNOWN_ERROR;
    }

    V1_0::DemuxFilterTsRecordEvent recordEvent;
    recordEvent = {
            .byteNumber = mFilterOutput.size(),
    };

    int size = mFilterEvent.events.size();
    mFilterEvent.events.resize(size + 1);
    mFilterEvent.events[size].tsRecord(recordEvent);

    if (mCallback_1_1 != nullptr) {
        V1_1::DemuxFilterTsRecordEventExt recordEventExt;
        recordEventExt = {
            .pts = (mPts == 0) ? time(NULL) * 900000 : mPts,
            .firstMbInSlice = 0,
        };
        mCallback_1_1->onFilterEvent_1_1(mFilterEvent, mFilterEventExt);
        mFilterEventExt.events.resize(0);
    } else if (mCallback != nullptr) {
        mCallback->onFilterEvent(mFilterEvent);
    }

    mFilterEvent.events.resize(0);

    mFilterOutput.clear();
    return Result::SUCCESS;
}

Result Filter::startPcrFilterHandler() {
    return Result::SUCCESS;
}

Result Filter::startTemiFilterHandler() {
    return Result::SUCCESS;
}

void Filter::handleSection(ts::SectionDemux& demux, const ts::Section& section) {
    ALOGV("%s", __FUNCTION__);
}

void Filter::handleTable(ts::SectionDemux& demux, const ts::BinaryTable& table) {
    if (mFilterStarted == false)  {
        return;
    }

    ts::SectionPtr secPtr = table.sectionAt(0);
    const uint8_t* sec_payload = secPtr->content();
    std::vector<uint8_t> data(sec_payload, sec_payload + secPtr->size() );
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (data.size() == 0) {
        return;
    }
    if (table.sourcePID() == SDT_PID) {
        ts::SDT* mSDT = new ts::SDT(*mDuckContext, table);
        if (!mSDT->isActual()) {
            delete mSDT;
            return;
        }
        delete mSDT;
    }
    if(table.tableId() == PMT_TABLE_ID) {
        if (mLastVersion != -1 && mLastVersion == table.version()) {
            ALOGD("[Filter] same version pid %d ", mTpid);
            return;
        }
    }

    if (!writeDataToFilterMQ(data)) {
        ALOGD("[Filter] FAiled to write FMQ");
        return;
    }

    int size = mFilterEvent.events.size();
    mFilterEvent.events.resize(size + 1);
    DemuxFilterSectionEvent secEvent;
    secEvent = {
            // temp dump meta data
            .tableId = table.tableId(),
            .version = table.version(),
            .sectionNum = static_cast<uint16_t>(table.sectionCount()),
            .dataLength = static_cast<uint16_t>(data.size()),
    };

    mLastVersion = table.version();
    mFilterEvent.events[size].section(secEvent);

    // After successfully write, send a callback and wait for the read to be done
    if (mCallback_1_1 != nullptr) {
        mCallback_1_1->onFilterEvent_1_1(mFilterEvent, mFilterEventExt);
        mFilterEventExt.events.resize(0);
    } else if (mCallback != nullptr) {
        mCallback->onFilterEvent(mFilterEvent);
    }

    mFilterEvent.events.resize(0);

    if (mCallback != nullptr) {
            mCallback->onFilterStatus(DemuxFilterStatus::DATA_READY);
    } else if (mCallback_1_1 != nullptr) {
            mCallback_1_1->onFilterStatus(DemuxFilterStatus::DATA_READY);
    }
}

bool Filter::writeSectionsAndCreateEvent(vector<uint8_t> data) {
    uint8_t b[188];

    for (int i = 0 ; i < 188; i++) {
       b[i] = data[i];
    }
    mSectionDemux->addPID(getTpid());
    ts::TSPacket *pkt = new ts::TSPacket();
    pkt->copyFrom(reinterpret_cast<void*> (b));
    mSectionDemux->feedPacket(*pkt);
    delete pkt;
    return true;
}

bool Filter::writeDataToFilterMQ(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mFilterMQ->write(data.data(), data.size())) {
        return true;
    }
    return false;
}

void Filter::attachFilterToRecord(const sp<Dvr> dvr) {
    mDvr = dvr;
}

void Filter::detachFilterFromRecord() {
    mDvr = nullptr;
}

int Filter::createAvIonFd(int size) {
    // Create an DMA-BUF fd and allocate an av fd mapped to a buffer to it.
    auto buffer_allocator = std::make_unique<BufferAllocator>();
    if (!buffer_allocator) {
        ALOGE("[Filter] Unable to create BufferAllocator object");
        return -1;
    }
    int av_fd = -1;
    av_fd = buffer_allocator->Alloc("system", size);
    if (av_fd < 0) {
        ALOGE("[Filter] Failed to create av fd %d", errno);
        return -1;
    }
    return av_fd;
}

uint8_t* Filter::getIonBuffer(int fd, int size, off_t *pa) {
    uint8_t* avBuf;
    if (mUsingSharedAvMem) {
        off_t pa_offset = mSharedAvMemOffset & ~(sysconf(_SC_PAGE_SIZE) - 1);
        off_t pa_alignment = mSharedAvMemOffset - pa_offset;
        off_t pa_aligned_size = size + pa_alignment;
        *pa = pa_alignment;

        avBuf = static_cast<uint8_t*>(
            mmap(NULL, pa_aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa_offset));

    } else {
        avBuf = static_cast<uint8_t*>(
                mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 /*offset*/));
    }
    if (avBuf == MAP_FAILED) {
        ALOGE("[Filter] fail to allocate buffer %d", errno);
        return NULL;
    }
    return avBuf;
}

native_handle_t* Filter::createNativeHandle(int fd) {
    native_handle_t* nativeHandle;
    if (fd < 0) {
        nativeHandle = native_handle_create(/*numFd*/ 0, 0);
    } else {
        nativeHandle = native_handle_create(/*numFd*/ 1, 0);
    }
    if (nativeHandle == NULL) {
        ALOGE("[Filter] Failed to create native_handle %d", errno);
        return NULL;
    }
    if (nativeHandle->numFds > 0) {
        nativeHandle->data[0] = dup(fd);
    }
    return nativeHandle;
}

Result Filter::createIndependentMediaEvents(vector<uint8_t> output) {
    int av_fd = createAvIonFd(output.size());
    if (av_fd == -1) {
        return Result::UNKNOWN_ERROR;
    }
    // copy the filtered data to the buffer
    uint8_t* avBuffer = getIonBuffer(av_fd, output.size(), 0);
    if (avBuffer == NULL) {
        return Result::UNKNOWN_ERROR;
    }
    memcpy(avBuffer, output.data(), output.size() * sizeof(uint8_t));

    native_handle_t* nativeHandle = createNativeHandle(av_fd);
    if (nativeHandle == NULL) {
        return Result::UNKNOWN_ERROR;
    }
    hidl_handle handle;
    handle.setTo(nativeHandle, /*shouldOwn=*/true);
    // Create a dataId and add a <dataId, av_fd> pair into the dataId2Avfd map
    uint64_t dataId = mLastUsedDataId++ /*createdUID*/;
    mDataId2Avfd[dataId] = dup(av_fd);

    // Create mediaEvent and send callback
    DemuxFilterMediaEvent mediaEvent;
    mediaEvent = {
            .avMemory = std::move(handle),
            .dataLength = static_cast<uint32_t>(output.size()),
            .avDataId = dataId,
    };
    if (mPts) {
        if(IS_32BIT)
            mediaEvent.pts = mPts / 90;
        else
            mediaEvent.pts = mPts;

        mPts = 0;
    }
    int size = mFilterEvent.events.size();
    mFilterEvent.events.resize(size + 1);
    mFilterEvent.events[size].media(mediaEvent);

    if (mCallback_1_1 != nullptr) {
        mCallback_1_1->onFilterEvent_1_1(mFilterEvent, mFilterEventExt);
        mFilterEventExt.events.resize(0);
    } else if (mCallback != nullptr) {
        mCallback->onFilterEvent(mFilterEvent);
    }

    mFilterEvent.events.resize(0);

    output.clear();
    mAvBufferCopyCount = 0;
    ::close(av_fd);
    if (DEBUG_FILTER) {
        ALOGD("[Filter] av data length %d", mediaEvent.dataLength);
    }
    return Result::SUCCESS;
}

Result Filter::createShareMemMediaEvents(vector<uint8_t> output) {
    off_t pa_alignment;
    if (mSharedAvMemOffset + output.size() >= BUFFER_SIZE_16M) {
        mSharedAvMemOffset = 0;
    }
    // copy the filtered data to the shared buffer
    uint8_t* sharedAvBuffer = getIonBuffer(mSharedAvMemHandle.getNativeHandle()->data[0],
                                           output.size(), &pa_alignment);

    if (sharedAvBuffer == NULL) {
         ALOGE("[Filter] sharedAvBuffer == NULL");
        return Result::UNKNOWN_ERROR;
    }
    memcpy(sharedAvBuffer + pa_alignment, output.data(), output.size() * sizeof(uint8_t));

    // Create a memory handle with numFds == 0
    native_handle_t* nativeHandle = createNativeHandle(-1);
    if (nativeHandle == NULL) {
        ALOGE("[Filter] nativeHandle == NULL");
        return Result::UNKNOWN_ERROR;
    }
    hidl_handle handle;
    handle.setTo(nativeHandle, /*shouldOwn=*/true);

    // Create mediaEvent and send callback
    DemuxFilterMediaEvent mediaEvent;
    mediaEvent = {
            .offset = static_cast<uint32_t>(mSharedAvMemOffset),
            .dataLength = static_cast<uint32_t>(output.size()),
            .avMemory = std::move(handle),
    };
    mSharedAvMemOffset += output.size();
    if (mPts) {
        if(IS_32BIT)
            mediaEvent.pts = mPts / 90;
        else
            mediaEvent.pts = mPts;

        mediaEvent.isPtsPresent = true;
    }

    mediaEvent.isSecureMemory = false;

    int size = mFilterEvent.events.size();
    mFilterEvent.events.resize(size + 1);
    mFilterEvent.events[size].media(mediaEvent);

    if (mCallback_1_1 != nullptr) {
        mCallback_1_1->onFilterEvent_1_1(mFilterEvent, mFilterEventExt);
        mFilterEventExt.events.resize(0);
    } else if (mCallback != nullptr) {
        mCallback->onFilterEvent(mFilterEvent);
    }

    mFilterEvent.events.resize(0);

    int result = munmap(sharedAvBuffer, output.size() + pa_alignment);
    output.clear();
    if (result == -1) {
         ALOGW("[Filter] unmap failed");
    }

    if (DEBUG_FILTER) {
        ALOGD("[Filter] shared av data length %d", mediaEvent.dataLength);
    }
    return Result::SUCCESS;
}

bool Filter::sameFile(int fd1, int fd2) {
    struct stat stat1, stat2;
    if (fstat(fd1, &stat1) < 0 || fstat(fd2, &stat2) < 0) {
        return false;
    }
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

DemuxFilterEvent Filter::createMediaEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].media({
            .streamId = 1,
            .isPtsPresent = true,
            .pts = 2,
            .dataLength = 3,
            .offset = 4,
            .isSecureMemory = true,
            .mpuSequenceNumber = 6,
            .isPesPrivateData = true,
    });

    event.events[0].media().extraMetaData.audio({
            .adFade = 1,
            .adPan = 2,
            .versionTextTag = 3,
            .adGainCenter = 4,
            .adGainFront = 5,
            .adGainSurround = 6,
    });

    int av_fd = createAvIonFd(BUFFER_SIZE_16M);
    if (av_fd == -1) {
        return event;
    }

    native_handle_t* nativeHandle = createNativeHandle(av_fd);
    if (nativeHandle == NULL) {
        ::close(av_fd);
        ALOGE("[Filter] Failed to create native_handle %d", errno);
        return event;
    }

    // Create a dataId and add a <dataId, av_fd> pair into the dataId2Avfd map
    uint64_t dataId = mLastUsedDataId++ /*createdUID*/;
    mDataId2Avfd[dataId] = dup(av_fd);
    event.events[0].media().avDataId = dataId;

    hidl_handle handle;
    handle.setTo(nativeHandle, /*shouldOwn=*/true);
    event.events[0].media().avMemory = std::move(handle);
    ::close(av_fd);
    return event;
}

DemuxFilterEvent Filter::createTsRecordEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    DemuxPid pid;
    pid.tPid(1);
    DemuxFilterTsRecordEvent::ScIndexMask mask;
    mask.sc(1);
    event.events[0].tsRecord({
            .pid = pid,
            .tsIndexMask = 1,
            .scIndexMask = mask,
            .byteNumber = 2,
    });
    return event;
}

V1_1::DemuxFilterEventExt Filter::createTsRecordEventExt() {
    V1_1::DemuxFilterEventExt event;
    event.events.resize(1);

    event.events[0].tsRecord({
            .pts = 1,
            .firstMbInSlice = 2,
    });
    return event;
}

DemuxFilterEvent Filter::createMmtpRecordEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].mmtpRecord({
            .scHevcIndexMask = 1,
            .byteNumber = 2,
    });
    return event;
}

V1_1::DemuxFilterEventExt Filter::createMmtpRecordEventExt() {
    V1_1::DemuxFilterEventExt event;
    event.events.resize(1);

    event.events[0].mmtpRecord({
            .pts = 1,
            .mpuSequenceNumber = 2,
            .firstMbInSlice = 3,
            .tsIndexMask = 4,
    });
    return event;
}

DemuxFilterEvent Filter::createSectionEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].section({
            .tableId = 1,
            .version = 2,
            .sectionNum = 3,
            .dataLength = 0,
    });
    return event;
}

DemuxFilterEvent Filter::createPesEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].pes({
            .streamId = static_cast<DemuxStreamId>(1),
            .dataLength = 1,
            .mpuSequenceNumber = 2,
    });
    return event;
}

DemuxFilterEvent Filter::createDownloadEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].download({
            .itemId = 1,
            .mpuSequenceNumber = 2,
            .itemFragmentIndex = 3,
            .lastItemFragmentIndex = 4,
            .dataLength = 0,
    });
    return event;
}

DemuxFilterEvent Filter::createIpPayloadEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].ipPayload({
            .dataLength = 0,
    });
    return event;
}

DemuxFilterEvent Filter::createTemiEvent() {
    DemuxFilterEvent event;
    event.events.resize(1);

    event.events[0].temi({.pts = 1, .descrTag = 2, .descrData = {3}});
    return event;
}

V1_1::DemuxFilterEventExt Filter::createMonitorEvent() {
    V1_1::DemuxFilterEventExt event;
    event.events.resize(1);

    V1_1::DemuxFilterMonitorEvent monitor;
    monitor.scramblingStatus(V1_1::ScramblingStatus::SCRAMBLED);
    event.events[0].monitorEvent(monitor);
    return event;
}

V1_1::DemuxFilterEventExt Filter::createRestartEvent() {
    V1_1::DemuxFilterEventExt event;
    event.events.resize(1);

    event.events[0].startId(mStartId);
    return event;
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
