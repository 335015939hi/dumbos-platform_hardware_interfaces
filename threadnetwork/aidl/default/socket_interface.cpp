/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the implementation for the Socket interface to radio
 * (RCP).
 */

#include "socket_interface.hpp"

#include <errno.h>
#include <openthread/error.h>
#include <openthread/logging.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include <string>

#include "lib/platform/exit_code.h"
#include "lib/spinel/spinel_interface.hpp"
#include "openthread/openthread-system.h"
#include "platform-posix.h"

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
    mInterfaceMetrics.mRcpInterfaceType = kSpinelInterfaceTypeSocket;
    CheckIfSocketIsOpen(aRadioUrl);
}

otError SocketInterface::Init(ReceiveFrameCallback aCallback, void* aCallbackContext,
                              RxFrameBuffer& aFrameBuffer) {
    otError error = OT_ERROR_NONE;
    struct stat st;

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

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

otError SocketInterface::SendFrame(const uint8_t* aFrame, uint16_t aLength) {
    otError error = OT_ERROR_NONE;
    Write(aFrame, aLength);

    if (IsSpinelResetCommand(aFrame, aLength)) {
        error = ResetConnection();
    }

    return error;
}

otError SocketInterface::WaitForFrame(uint64_t aTimeoutUs) {
    otError error = OT_ERROR_NONE;
    struct timeval timeout;
    timeout.tv_sec = static_cast<time_t>(aTimeoutUs / US_PER_S);
    timeout.tv_usec = static_cast<suseconds_t>(aTimeoutUs % US_PER_S);

    fd_set read_fds;
    fd_set error_fds;
    int rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&error_fds);
    FD_SET(mSockFd, &read_fds);
    FD_SET(mSockFd, &error_fds);

    rval = select(mSockFd + 1, &read_fds, nullptr, &error_fds, &timeout);

    if (rval > 0) {
        if (FD_ISSET(mSockFd, &read_fds)) {
            Read();
        } else if (FD_ISSET(mSockFd, &error_fds)) {
            DieNowWithMessage("NCP error", OT_EXIT_FAILURE);
        } else {
            DieNow(OT_EXIT_FAILURE);
        }
    } else if (rval == 0) {
        ExitNow(error = OT_ERROR_RESPONSE_TIMEOUT);
    } else if (errno != EINTR) {
        DieNowWithMessage("wait response", OT_EXIT_FAILURE);
    }

exit:
    return error;
}

void SocketInterface::UpdateFdSet(void* aMainloopContext) {
    otSysMainloopContext* context = reinterpret_cast<otSysMainloopContext*>(aMainloopContext);

    assert(context != nullptr);

    FD_SET(mSockFd, &context->mReadFdSet);

    if (context->mMaxFd < mSockFd) {
        context->mMaxFd = mSockFd;
    }
}

void SocketInterface::Process(const void* aMainloopContext) {
    const otSysMainloopContext* context =
            reinterpret_cast<const otSysMainloopContext*>(aMainloopContext);

    assert(context != nullptr);

    if (FD_ISSET(mSockFd, &context->mReadFdSet)) {
        Read();
    }
}

otError SocketInterface::ResetConnection(void) {
    otError error = OT_ERROR_NONE;
    uint64_t end;

    if (mRadioUrl.HasParam("socket-reset")) {
        usleep(static_cast<useconds_t>(kRemoveRcpDelay) * US_PER_MS);
        CloseFile();

        end = otPlatTimeGet() + kResetTimeout * US_PER_MS;
        do {
            mSockFd = OpenFile(mRadioUrl);
            if (mSockFd != -1) {
                ExitNow();
            }
            usleep(static_cast<useconds_t>(kOpenFileDelay) * US_PER_MS);
        } while (end > otPlatTimeGet());

        otLogCritPlat("Failed to reopen Socket connection after resetting the RCP device.");
        error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

void SocketInterface::Read(void) {
    uint8_t buffer[kMaxFrameSize];
    ssize_t rval;

    rval = read(mSockFd, buffer, sizeof(buffer));

    if (rval > 0) {
        ProcessReceivedData(buffer, static_cast<uint16_t>(rval));
    } else if ((rval < 0) && (errno != EAGAIN) && (errno != EINTR)) {
        DieNow(OT_EXIT_ERROR_ERRNO);
    } else if (rval == 0) {
        otLogCritPlat("Socket connection is closed by remote.");
        exit(OT_EXIT_FAILURE);
    }
}

void SocketInterface::Write(const uint8_t* aFrame, uint16_t aLength) {
    otError error = OT_ERROR_NONE;

    ssize_t rval = write(mSockFd, aFrame, aLength);
    if (rval <= 0) {
        VerifyOrDie((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR),
                    OT_EXIT_ERROR_ERRNO);
    }
}

void SocketInterface::ProcessReceivedData(const uint8_t* aBuffer, uint16_t aLength) {
    otError error = OT_ERROR_PARSE;
    while (aLength--) {
        uint8_t byte = *aBuffer++;
        if (mReceiveFrameBuffer->CanWrite(sizeof(uint8_t))) {
            IgnoreError(mReceiveFrameBuffer->WriteByte(byte));
        } else {
            HandleSocketFrame(this, OT_ERROR_NO_BUFS);
        }
    }
    HandleSocketFrame(this, OT_ERROR_NONE);
}

void SocketInterface::HandleSocketFrame(void* aContext, otError aError) {
    static_cast<SocketInterface*>(aContext)->HandleSocketFrame(aError);
}

void SocketInterface::HandleSocketFrame(otError aError) {
    VerifyOrExit((mReceiveFrameCallback != nullptr) && (mReceiveFrameBuffer != nullptr));

    if (aError == OT_ERROR_NONE) {
        mReceiveFrameCallback(mReceiveFrameContext);
    } else {
        mReceiveFrameBuffer->DiscardFrame();
        otLogWarnPlat("Error decoding hdlc frame: %s", otThreadErrorToString(aError));
    }

exit:
    return;
}

int SocketInterface::OpenFile(const ot::Url::Url& aRadioUrl) {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd == -1) {
        perror("open socket failed");
        ExitNow();
    }

    sockaddr_un server_address;
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, aRadioUrl.GetPath());

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) ==
        -1) {
        perror("connect failed");
        close(fd);
        fd = -1;
    }

exit:
    return fd;
}

void SocketInterface::CloseFile(void) {
    VerifyOrExit(mSockFd != -1);

    VerifyOrExit(0 == close(mSockFd), perror("close RCP"));
    VerifyOrExit(wait(nullptr) != -1 || errno == ECHILD, perror("wait RCP"));

    mSockFd = -1;

exit:
    return;
}

void SocketInterface::CheckIfSocketIsOpen(const ot::Url::Url& aRadioUrl) {
    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify_init fail");
        exit(OT_EXIT_FAILURE);
    }

    std::string socket_path(aRadioUrl.GetPath());
    auto last_slash_idx = socket_path.find_last_of('/');
    if (last_slash_idx == std::string::npos) {
        perror("Invalid socket path");
        exit(OT_EXIT_FAILURE);
    }

    auto folder_path = socket_path.substr(0, last_slash_idx);

    int wd = inotify_add_watch(inotify_fd, folder_path.c_str(), IN_CREATE);
    if (wd == -1) {
        perror("inotify_add_watch fail");
        exit(OT_EXIT_FAILURE);
    }

    struct stat st;
    if (stat(aRadioUrl.GetPath(), &st) == 0) {
        otLogInfoPlat("Socket file: %s is created", aRadioUrl.GetPath());
        return;
    }

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(inotify_fd, &fds);
        struct timeval timeout = {kMaxSelectTime / 1000, (kMaxSelectTime % 1000) * 1000};

        otLogInfoPlat("Waiting for socket file %s be created...", aRadioUrl.GetPath());

        int rval = select(inotify_fd + 1, &fds, nullptr, nullptr, &timeout);
        if (rval < 0) {
            perror("Select for socket notify fd fail.");
            exit(OT_EXIT_FAILURE);
        }

        if (rval == 0 && stat(aRadioUrl.GetPath(), &st) == 0) {
            break;
        }

        if (FD_ISSET(inotify_fd, &fds)) {
            char buffer[4096];
            ssize_t bytes_read = read(inotify_fd, buffer, sizeof(buffer));
            if (bytes_read == -1) {
                perror("Read for socket notify fd fail.");
                exit(OT_EXIT_FAILURE);
            }

            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(buffer);
            if (event->mask & IN_CREATE) {
                if (stat(aRadioUrl.GetPath(), &st) == 0) {
                    break;
                }
            }
        }
    }

    close(inotify_fd);
    otLogInfoPlat("Socket file: %s is created", aRadioUrl.GetPath());
}

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
