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

#include "DescramblerTests.h"

using namespace std;

AssertionResult DescramblerTests::createCasPlugin(int32_t caSystemId) {
    if (isAidl) {
        bool isSupported = false;
        auto status = mMediaCasService->isSystemIdSupported(caSystemId, &isSupported);
        if (!status.isOk() || !isSupported) {
            ALOGW("[vts] Failed to check isSystemIdSupported.");
            return failure();
        }

        mCasListener = ndk::SharedRefBase::make<MediaCasListener>();
        auto pluginStatus = mMediaCasService->createPlugin(caSystemId, mCasListener, &mCas);
        if (!pluginStatus.isOk()) {
            ALOGW("[vts] Failed to CAS createPlugin.");
            return failure();
        }
        if (mCas == nullptr) {
            ALOGW("[vts] Failed to get ICas.");
            return failure();
        }
        return success();
    } else {
        auto status = mMediaCasServiceHidl->isSystemIdSupported(caSystemId);
        if (!status.isOk() || !status) {
            ALOGW("[vts] Failed to check isSystemIdSupported.");
            return failure();
        }

        mCasListenerHidl = new MediaCasListenerHidl();
        auto pluginStatus = mMediaCasServiceHidl->createPluginExt(caSystemId, mCasListenerHidl);
        if (!pluginStatus.isOk()) {
            ALOGW("[vts] Failed to createPluginExt.");
            return failure();
        }
        mCasHidl = ICasHidl::castFrom(pluginStatus);
        if (mCasHidl == nullptr) {
            ALOGW("[vts] Failed to get ICas.");
            return failure();
        }
        return success();
    }
}

AssertionResult DescramblerTests::openCasSession(vector<uint8_t>& sessionId,
                                                 vector<uint8_t>& hidlPvtData) {
    if (isAidl) {
        SessionIntent intent = SessionIntent::LIVE;
        ScramblingMode mode = ScramblingMode::RESERVED;
        auto openStatus = mCas->openSession(intent, mode, &sessionId);
        if (!openStatus.isOk()) {
            ALOGW("[vts] Failed to open cas session.");
            mCas->closeSession(sessionId);
            return failure();
        }

        if (hidlPvtData.size() > 0) {
            auto status = mCas->setSessionPrivateData(sessionId, hidlPvtData);
            if (!status.isOk()) {
                ALOGW("[vts] Failed to set session private data");
                mCas->closeSession(sessionId);
                return failure();
            }
        }

        return success();
    } else {
        SessionIntentHidl intent = SessionIntentHidl::LIVE;
        ScramblingModeHidl mode = ScramblingModeHidl::RESERVED;
        StatusHidl sessionStatus;
        auto returnVoid = mCasHidl->openSession_1_2(
                intent, mode, [&](StatusHidl status, const hidl_vec<uint8_t>& id) {
                    sessionStatus = status;
                    sessionId = id;
                });
        if (!returnVoid.isOk() || (sessionStatus != StatusHidl::OK)) {
            ALOGW("[vts] Failed to open cas session.");
            mCasHidl->closeSession(sessionId);
            return failure();
        }

        if (hidlPvtData.size() > 0) {
            auto status = mCasHidl->setSessionPrivateData(sessionId, hidlPvtData);
            if (status != android::hardware::cas::V1_0::Status::OK) {
                ALOGW("[vts] Failed to set session private data");
                mCasHidl->closeSession(sessionId);
                return failure();
            }
        }

        return success();
    }
}

AssertionResult DescramblerTests::getKeyToken(int32_t caSystemId, string& provisonStr,
                                              vector<uint8_t>& hidlPvtData,
                                              vector<uint8_t>& token) {
    if (createCasPlugin(caSystemId) != success()) {
        ALOGW("[vts] createCasPlugin failed.");
        return failure();
    }

    if (provisonStr.size() > 0) {
        if (isAidl) {
            auto returnStatus = mCas->provision(provisonStr);
            if (!returnStatus.isOk()) {
                ALOGW("[vts] provision failed.");
                return failure();
            }
        } else {
            auto returnStatus = mCasHidl->provision(hidl_string(provisonStr));
            if (returnStatus != android::hardware::cas::V1_0::Status::OK) {
                ALOGW("[vts] provision failed.");
                return failure();
            }
        }
    }

    return openCasSession(token, hidlPvtData);
}

AssertionResult DescramblerTests::openDescrambler(int32_t demuxId) {
    ndk::ScopedAStatus status;
    status = mService->openDescrambler(&mDescrambler);
    if (!status.isOk()) {
        ALOGW("[vts] openDescrambler failed.");
        return failure();
    }

    status = mDescrambler->setDemuxSource(demuxId);
    if (!status.isOk()) {
        ALOGW("[vts] setDemuxSource failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::setKeyToken(vector<uint8_t>& token) {
    ndk::ScopedAStatus status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->setKeyToken(token);
    if (!status.isOk()) {
        ALOGW("[vts] setKeyToken failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::addPid(DemuxPid pid,
                                         std::shared_ptr<IFilter> optionalSourceFilter) {
    ndk::ScopedAStatus status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->addPid(pid, optionalSourceFilter);
    if (!status.isOk()) {
        ALOGW("[vts] addPid failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::removePid(DemuxPid pid,
                                            std::shared_ptr<IFilter> optionalSourceFilter) {
    ndk::ScopedAStatus status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->removePid(pid, optionalSourceFilter);
    if (!status.isOk()) {
        ALOGW("[vts] removePid failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::closeDescrambler() {
    ndk::ScopedAStatus status;
    if (!mDescrambler) {
        ALOGW("[vts] Descrambler is not opened yet.");
        return failure();
    }

    status = mDescrambler->close();
    mDescrambler = nullptr;
    if (!status.isOk()) {
        ALOGW("[vts] close Descrambler failed.");
        return failure();
    }

    return success();
}

AssertionResult DescramblerTests::getDemuxPidFromFilterSettings(DemuxFilterType type,
                                                                const DemuxFilterSettings& settings,
                                                                DemuxPid& pid) {
    switch (type.mainType) {
        case DemuxFilterMainType::TS:
            if (type.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                        DemuxTsFilterType::AUDIO ||
                type.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                        DemuxTsFilterType::VIDEO) {
                pid.set<DemuxPid::Tag::tPid>(settings.get<DemuxFilterSettings::Tag::ts>().tpid);
            } else {
                ALOGW("[vts] Not a media ts filter!");
                return failure();
            }
            break;
        case DemuxFilterMainType::MMTP:
            if (type.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>() ==
                        DemuxMmtpFilterType::AUDIO ||
                type.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>() ==
                        DemuxMmtpFilterType::VIDEO) {
                pid.set<DemuxPid::Tag::mmtpPid>(
                        settings.get<DemuxFilterSettings::Tag::mmtp>().mmtpPid);
            } else {
                ALOGW("[vts] Not a media mmtp filter!");
                return failure();
            }
            break;
        default:
            ALOGW("[vts] Not a media filter!");
            return failure();
    }
    return success();
}
