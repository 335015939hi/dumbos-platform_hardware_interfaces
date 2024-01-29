/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
 * @file
 *   This file includes the implementation for the Socket interface to radio
 * (RCP).
 */

#include "socket_interface.hpp"

#include <errno.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "common/code_utils.hpp"
#include "lib/platform/exit_code.h"
#include "openthread/openthread-system.h"

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

SocketInterface::SocketInterface(const ot::Url::Url& aRadioUrl)
    : mReceiveFrameCallback(nullptr),
      mReceiveFrameContext(nullptr),
      mReceiveFrameBuffer(nullptr),
      mSockFd(-1),
      mRadioUrl(aRadioUrl) {
    memset(&mInterfaceMetrics, 0, sizeof(mInterfaceMetrics));
    mInterfaceMetrics.mRcpInterfaceType = kSpinelInterfaceTypeVendor;
}

otError SocketInterface::Init(ReceiveFrameCallback aCallback, void* aCallbackContext,
                              RxFrameBuffer& aFrameBuffer) {
    otError error = OT_ERROR_NONE;
    struct stat st;

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

    WaitForSocketFileCreated(mRadioUrl.GetPath());

    VerifyOrDie(stat(mRadioUrl.GetPath(), &st) == 0, OT_EXIT_ERROR_ERRNO);

    if (S_ISSOCK(st.st_mode)) {
        mSockFd = OpenFile(mRadioUrl);
        VerifyOrExit(mSockFd != -1, error = OT_ERROR_FAILED);
    } else {
        otLogCritPlat("Radio file '%s' not supported", mRadioUrl.GetPath());
        ExitNow(error = OT_ERROR_FAILED);
    }

    mReceiveFrameCallback = aCallback;
    mReceiveFrameContext = aCallbackContext;
    mReceiveFrameBuffer = &aFrameBuffer;

exit:
    return error;
}

SocketInterface::~SocketInterface(void) {
    Deinit();
}

void SocketInterface::Deinit(void) {
    CloseFile();

    mReceiveFrameCallback = nullptr;
    mReceiveFrameContext = nullptr;
    mReceiveFrameBuffer = nullptr;
}

void SocketInterface::UpdateFdSet(void* aMainloopContext) {
    otSysMainloopContext* context = reinterpret_cast<otSysMainloopContext*>(aMainloopContext);

    assert(context != nullptr);

    FD_SET(mSockFd, &context->mReadFdSet);

    if (context->mMaxFd < mSockFd) {
        context->mMaxFd = mSockFd;
    }
}

int SocketInterface::OpenFile(const ot::Url::Url& aRadioUrl) {
    int fd = -1;
    sockaddr_un server_address;

    VerifyOrExit(sizeof(server_address.sun_path) > strlen(aRadioUrl.GetPath()),
                 otLogCritPlat("Invalid file path legnth"));
    strncpy(server_address.sun_path, aRadioUrl.GetPath(), sizeof(server_address.sun_path));
    server_address.sun_family = AF_UNIX;

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    VerifyOrExit(fd != -1, otLogCritPlat("open(): errno=%s", strerror(errno)));

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) ==
        -1) {
        otLogCritPlat("connect(): errno=%s", strerror(errno));
        close(fd);
        fd = -1;
    }

exit:
    return fd;
}

void SocketInterface::CloseFile(void) {
    VerifyOrExit(mSockFd != -1);

    VerifyOrExit(0 == close(mSockFd), otLogCritPlat("close(): errno=%s", strerror(errno)));
    VerifyOrExit(wait(nullptr) != -1 || errno == ECHILD,
                 otLogCritPlat("wait(): errno=%s", strerror(errno)));

    mSockFd = -1;

exit:
    return;
}

void SocketInterface::WaitForSocketFileCreated(const char* aPath) {
    if (IsSocketFileExisted(aPath)) {
        return;
    }

    int inotifyFd = inotify_init();

    VerifyOrDie(inotifyFd != -1, OT_EXIT_ERROR_ERRNO);

    std::string socketPath(aPath);
    auto lastSlashIdx = socketPath.find_last_of('/');

    VerifyOrDie(lastSlashIdx != std::string::npos, OT_EXIT_ERROR_ERRNO);

    auto folderPath = socketPath.substr(0, lastSlashIdx);
    int wd = inotify_add_watch(inotifyFd, folderPath.c_str(), IN_CREATE);

    VerifyOrDie(wd != -1, OT_EXIT_ERROR_ERRNO);

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(inotifyFd, &fds);
        struct timeval timeout = {kMaxSelectTimeMs / 1000, (kMaxSelectTimeMs % 1000) * 1000};

        otLogInfoPlat("Waiting for socket file %s be created...", aPath);

        int rval = select(inotifyFd + 1, &fds, nullptr, nullptr, &timeout);
        VerifyOrDie(rval >= 0, OT_EXIT_ERROR_ERRNO);

        if (rval == 0 && IsSocketFileExisted(aPath)) {
            break;
        }

        if (FD_ISSET(inotifyFd, &fds)) {
            char buffer[sizeof(struct inotify_event)];
            ssize_t bytesRead = read(inotifyFd, buffer, sizeof(buffer));

            VerifyOrDie(bytesRead >= 0, OT_EXIT_ERROR_ERRNO);

            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(buffer);
            if ((event->mask & IN_CREATE) && IsSocketFileExisted(aPath)) {
                break;
            }
        }
    }

    close(inotifyFd);
}

bool SocketInterface::IsSocketFileExisted(const char* aPath) {
    struct stat st;
    if (stat(aPath, &st) == 0) {
        otLogInfoPlat("Socket file: %s is created", aPath);
        return true;
    }
    return false;
}

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
