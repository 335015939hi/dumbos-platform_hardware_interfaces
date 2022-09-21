/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 icensed under the Apache License, Version 2.0 (the "License");
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

#define LOG_TAG "android.hardware.cas-CasImpl"

#include <media/cas/CasAPI.h>
#include <utils/Log.h>

#include "CasImpl.h"
#include "SharedLibrary.h"
#include "TypeConvert.h"

namespace android {
namespace hardware {
namespace cas {
namespace implementation {

CasImpl::CasImpl(const std::shared_ptr<ICasListener>& listener) : mListener(listener) {
    ALOGV("CTOR");
}

CasImpl::~CasImpl() {
    ALOGV("DTOR");
    release();
}

// static
void CasImpl::OnEvent(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onEvent(event, arg, data, size);
}

// static
void CasImpl::CallBackExt(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size,
                          const CasSessionId* sessionId) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onEvent(sessionId, event, arg, data, size);
}

// static
void CasImpl::StatusUpdate(void* appData, int32_t event, int32_t arg) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onStatusUpdate(event, arg);
}

void CasImpl::init(CasPlugin* plugin) {
    std::shared_ptr<CasPlugin> holder(plugin);
    std::atomic_store(&mPluginHolder, holder);
}

void CasImpl::onEvent(int32_t event, int32_t arg, uint8_t* data, size_t size) {
    if (mListener == NULL) {
        return;
    }

    std::vector<uint8_t> eventData;
    if (data != NULL) {
        eventData.assign(data, data + size);
    }

    mListener->onEvent(event, arg, eventData);
}

void CasImpl::onEvent(const CasSessionId* sessionId, int32_t event, int32_t arg, uint8_t* data,
                      size_t size) {
    if (mListener == NULL) {
        return;
    }

    std::vector<uint8_t> eventData;
    if (data != NULL) {
        eventData.assign(data, data + size);
    }

    if (sessionId != NULL) {
        mListener->onSessionEvent(*sessionId, event, arg, eventData);
    } else {
        mListener->onEvent(event, arg, eventData);
    }
}

void CasImpl::onStatusUpdate(int32_t event, int32_t arg) {
    if (mListener == NULL) {
        return;
    }
    mListener->onStatusUpdate(static_cast<StatusEvent>(event), arg);
}

::ndk::ScopedAStatus CasImpl::setPluginStatusUpdateCallback() {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setStatusCallback(&CasImpl::StatusUpdate));
}

::ndk::ScopedAStatus CasImpl::setPrivateData(const std::vector<uint8_t>& pvtData) {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setPrivateData(pvtData));
}

::ndk::ScopedAStatus CasImpl::openSession(SessionIntent intent, ScramblingMode mode,
                                          std::vector<uint8_t>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);
    std::vector<uint8_t> sessionId;

    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    status_t err = INVALID_OPERATION;
    if (holder.get() != nullptr) {
        err = holder->openSession(static_cast<uint32_t>(intent), static_cast<uint32_t>(mode),
                                  &sessionId);
        holder.reset();
    }

    _aidl_return = &sessionId;
    return toStatus(err);
}

::ndk::ScopedAStatus CasImpl::setSessionPrivateData(const std::vector<uint8_t>& sessionId,
                                                    const std::vector<uint8_t>& pvtData) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).string());
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setSessionPrivateData(sessionId, pvtData));
}

::ndk::ScopedAStatus CasImpl::closeSession(const std::vector<uint8_t>& sessionId) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).string());
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->closeSession(sessionId));
}

::ndk::ScopedAStatus CasImpl::processEcm(const std::vector<uint8_t>& sessionId,
                                         const std::vector<uint8_t>& ecm) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).string());
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->processEcm(sessionId, ecm));
}

::ndk::ScopedAStatus CasImpl::processEmm(const std::vector<uint8_t>& emm) {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->processEmm(emm));
}

::ndk::ScopedAStatus CasImpl::sendEvent(int32_t event, int32_t arg,
                                        const std::vector<uint8_t>& eventData) {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->sendEvent(event, arg, eventData);
    return toStatus(err);
}

::ndk::ScopedAStatus CasImpl::sendSessionEvent(const std::vector<uint8_t>& sessionId, int32_t event,
                                               int32_t arg, const std::vector<uint8_t>& eventData) {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->sendSessionEvent(sessionId, event, arg, eventData);
    return toStatus(err);
}

::ndk::ScopedAStatus CasImpl::provision(const std::string& provisionString) {
    ALOGV("%s: provisionString=%s", __FUNCTION__, provisionString.c_str());
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->provision(String8(provisionString.c_str())));
}

::ndk::ScopedAStatus CasImpl::refreshEntitlements(int32_t refreshType,
                                                  const std::vector<uint8_t>& refreshData) {
    ALOGV("%s", __FUNCTION__);
    std::shared_ptr<CasPlugin> holder = std::atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->refreshEntitlements(refreshType, refreshData);
    return toStatus(err);
}

::ndk::ScopedAStatus CasImpl::release() {
    ALOGV("%s: plugin=%p", __FUNCTION__, mPluginHolder.get());

    std::shared_ptr<CasPlugin> holder(nullptr);
    std::atomic_store(&mPluginHolder, holder);

    return ndk::ScopedAStatus::ok();
}

}  // namespace implementation
}  // namespace cas
}  // namespace hardware
}  // namespace android
