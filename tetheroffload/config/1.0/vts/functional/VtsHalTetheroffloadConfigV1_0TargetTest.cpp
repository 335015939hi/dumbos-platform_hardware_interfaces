/*
 *Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are
 *met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define LOG_TAG "VtsOffloadConfigV1_0TargetTest"

#include <android-base/unique_fd.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <log/log.h>
#include <VtsHalHidlTargetTestBase.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::hidl_handle;
using android::hardware::Return;
using android::hardware::Void;
using android::sp;
using android::hardware::hidl_string;

inline const sockaddr * asSockaddr(const sockaddr_nl *nladdr) {
    return reinterpret_cast<const sockaddr *>(nladdr);
}

int conntrackSocket(unsigned groups) {
    android::base::unique_fd s(socket(AF_NETLINK, SOCK_DGRAM, NETLINK_NETFILTER));
    if (s.get() < 0) {
        return -errno;
    }

    const struct sockaddr_nl bind_addr = {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = 0,
        .nl_groups = groups,
    };
    if (::bind(s.get(), asSockaddr(&bind_addr), sizeof(bind_addr)) != 0) {
        return -errno;
    }

    const struct sockaddr_nl kernel_addr = {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = 0,
        .nl_groups = groups,
    };
    if (connect(s.get(), asSockaddr(&kernel_addr), sizeof(kernel_addr)) != 0) {
        return -errno;
    }

    return s.release();
}

class OffloadConfigHidlTest : public testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        config = testing::VtsHalHidlTargetTestBase::getService<IOffloadConfig>();
        ASSERT_NE(nullptr, config.get()) << "Could not get HIDL instance";
    }

    virtual void TearDown() override {}

    sp<IOffloadConfig> config;
};

/**
 * Ensure handles can be set with correct socket options.
 */
TEST_F(OffloadConfigHidlTest, TestSetHandles) {
    android::base::unique_fd
        fd1(conntrackSocket(NFNLGRP_CONNTRACK_NEW | NFNLGRP_CONNTRACK_DESTROY)),
        fd2(conntrackSocket(NFNLGRP_CONNTRACK_UPDATE | NFNLGRP_CONNTRACK_DESTROY));

    if (fd1.get() < 0 || fd2.get() < 0) {
        ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
        return;
    }

    native_handle_t* nativeHandle1 = native_handle_create(1, 0);
    nativeHandle1->data[0] = fd1;
    hidl_handle h1 = hidl_handle(nativeHandle1);

    native_handle_t* nativeHandle2 = native_handle_create(1, 0);
    nativeHandle2->data[0] = fd2;
    hidl_handle h2 = hidl_handle(nativeHandle2);

    if(h1->numFds == 1 && h2->numFds == 1) {
        ALOGE("Num FDs for both is 1");
    } else {
        ALOGE("num FDs not 1: %d %d", h1->numFds, h2->numFds);
    }

    auto cb = [&](bool success, const hidl_string& errMsg) {
        ASSERT_TRUE(success) << errMsg.c_str();
    };

    Return<void> ret = config->setHandles(h1, h2, cb);
    ASSERT_TRUE(ret.isOk());
}

/**
 * Passing a handle without an associated FD should return an
 * error (Failed Input Checks). Check that this occurs with FD1.
 */
TEST_F(OffloadConfigHidlTest, TestSetHandleNone) {
    native_handle_t* nativeHandle1 = native_handle_create(0, 0);
    hidl_handle h1 = hidl_handle(nativeHandle1);
    native_handle_t* nativeHandle2 = native_handle_create(0, 0);
    hidl_handle h2 = hidl_handle(nativeHandle2);

    auto cb = [&](bool success, const hidl_string& errMsg) {
        ASSERT_FALSE(success) << errMsg.c_str();
    };

    Return<void> ret = config->setHandles(h1, h2, cb);
    ASSERT_TRUE(ret.isOk());
}

/**
 * Passing a handle without an associated FD should return an
 * error (Failed Input Checks). Check that this occurs with FD2.
 */
TEST_F(OffloadConfigHidlTest, TestSetHandle1Only) {
    android::base::unique_fd
        fd1(conntrackSocket(NFNLGRP_CONNTRACK_NEW | NFNLGRP_CONNTRACK_DESTROY));

    native_handle_t* nativeHandle1 = native_handle_create(1, 0);
    nativeHandle1->data[0] = fd1;
    hidl_handle h1 = hidl_handle(nativeHandle1);
    native_handle_t* nativeHandle2 = native_handle_create(0, 0);
    hidl_handle h2 = hidl_handle(nativeHandle2);

    auto cb = [&](bool success, const hidl_string& errMsg) {
        ASSERT_FALSE(success) << errMsg.c_str();
    };

    Return<void> ret = config->setHandles(h1, h2, cb);
    ASSERT_TRUE(ret.isOk());
}

/**
 * Passing a handle without an associated FD should return an
 * error (Failed Input Checks). Check that this occurs with FD1.
 */
TEST_F(OffloadConfigHidlTest, TestSetHandle2Only) {
    android::base::unique_fd
        fd2(conntrackSocket(NFNLGRP_CONNTRACK_UPDATE | NFNLGRP_CONNTRACK_DESTROY));

    native_handle_t* nativeHandle1 = native_handle_create(0, 0);
    hidl_handle h1 = hidl_handle(nativeHandle1);
    native_handle_t* nativeHandle2 = native_handle_create(1, 0);
    nativeHandle2->data[0] = fd2;
    hidl_handle h2 = hidl_handle(nativeHandle2);

    auto cb = [&](bool success, const hidl_string& errMsg) {
        ASSERT_FALSE(success) << errMsg.c_str();
    };

    Return<void> ret = config->setHandles(h1, h2, cb);
    ASSERT_TRUE(ret.isOk());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
