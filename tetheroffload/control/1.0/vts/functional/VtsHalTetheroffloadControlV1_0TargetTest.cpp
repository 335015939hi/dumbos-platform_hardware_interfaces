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
#define LOG_TAG "VtsOffloadControlV1_0TargetTest"

#include <android-base/unique_fd.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.0/types.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <log/log.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_0::IOffloadControl;
using android::hardware::tetheroffload::control::V1_0::OffloadCallbackEvent;
using android::hardware::tetheroffload::control::V1_0::NetworkProtocol;
using android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair;
using android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate;
using android::hardware::Return;
using android::hardware::Void;
using android::sp;

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
    if (::bind(s.get(), asSockaddr(&bind_addr), sizeof(bind_addr)) < 0) {
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

constexpr char kCallbackOnEvent[] = "onEvent";
constexpr char kCallbackUpdateTimeout[] = "updateTimeout";

class TetheringOffloadCallbackArgs {
   public:
    OffloadCallbackEvent last_event;
    NatTimeoutUpdate last_params;
};

class OffloadControlHidlTest : public testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        control = testing::VtsHalHidlTargetTestBase::getService<IOffloadControl>();
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        control_cb = new TetheringOffloadCallback();
        ASSERT_NE(nullptr, control.get()) << "Could not get get offload callback";

        /*
         * Config must be set with correct socket options in order for
         * any control options to be set.
         */
        config = testing::VtsHalHidlTargetTestBase::getService<IOffloadConfig>();
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        android::base::unique_fd
            fd1(conntrackSocket(NFNLGRP_CONNTRACK_NEW | NFNLGRP_CONNTRACK_DESTROY)),
            fd2(conntrackSocket(NFNLGRP_CONNTRACK_UPDATE | NFNLGRP_CONNTRACK_DESTROY));

        native_handle_t* nativeHandle1 = native_handle_create(1, 0);
        nativeHandle1->data[0] = fd1;
        hidl_handle h1 = hidl_handle(nativeHandle1);

        native_handle_t* nativeHandle2 = native_handle_create(1, 0);
        nativeHandle2->data[0] = fd2;
        hidl_handle h2 = hidl_handle(nativeHandle2);

        auto config_cb = [&](bool config_success, std::string errMsg) {
            if(!config_success) {
                ALOGI("Config CB Error message: %s", errMsg.c_str());
            }
            ASSERT_TRUE(config_success);
        };

        Return<void> ret = config->setHandles(h1, h2, config_cb);
        ASSERT_TRUE(ret.isOk());

        auto init_cb = [&](bool success, std::string errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };
        ret = control->initOffload(control_cb, init_cb);
        ASSERT_TRUE(ret.isOk());
    }

    virtual void TearDown() override {
        auto cb = [&](bool success, const hidl_string& errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };
        Return<void> ret = control->stopOffload(cb);
        ASSERT_TRUE(ret.isOk());

        control_cb.clear();
    }

    /* Callback class for data & Event. */
    class TetheringOffloadCallback
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgs>,
          public ITetheringOffloadCallback {
       public:
        TetheringOffloadCallback(){};

        virtual ~TetheringOffloadCallback() = default;

        /* onEvent callback function - Called when an asynchronous
         * event is generated by the hardware management process.
         **/
        Return<void> onEvent(OffloadCallbackEvent event) override {
            TetheringOffloadCallbackArgs args;
            args.last_event = event;
            NotifyFromCallback(kCallbackOnEvent, args);
            return Void();
        };

        Return<void> updateTimeout(const NatTimeoutUpdate &params) override {
            TetheringOffloadCallbackArgs args;
            args.last_params = params;
            NotifyFromCallback(kCallbackUpdateTimeout, args);
            return Void();
        };
    };

    sp<IOffloadConfig> config;
    sp<IOffloadControl> control;
    sp<TetheringOffloadCallback> control_cb;
};

/**
 * InitOffloadTwice:
 * Calls initOffload() again. First initOffload is in setUp().
 * Check that initOffload returns false, since it was already
 * called from SetUp().
 */
TEST_F(OffloadControlHidlTest, InitOffloadTwice) {
    auto cb = [&](bool success, std::string errMsg) {
                  ALOGI("Error message: %s", errMsg.c_str());
                  ASSERT_FALSE(success);
              };
    Return<void> ret = control->initOffload(control_cb, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * StopOffload:
 * Calls StopOffload()
 * Check that stopOffload after init is successful.
 */
TEST_F(OffloadControlHidlTest, StopOffload) {
    /* Empty function tested as part of tearDown */
}

/**
 * setLocalPrefixes:
 * Calls setLocalPrefixes(). This is an unused function which
 * always returns true. The functionality of this function moved
 * to the Config HAL function setHandles().
 */
TEST_F(OffloadControlHidlTest, SetLocalPrefixes) {
    auto cb = [&](bool success, std::string errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };
    hidl_vec<hidl_string> prefixes;
    Return<void> ret = control->setLocalPrefixes(prefixes, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * getForwardedStats:
 * Calls getForwardedStats(). Stats should always be 0
 * since there is no data traffic.
 */
TEST_F(OffloadControlHidlTest, GetForwardedStats) {
    auto cb = [&](uint64_t rxBytes, uint64_t txBytes) {
                  EXPECT_EQ((uint64_t) 0, rxBytes);
                  EXPECT_EQ((uint64_t) 0, txBytes);
              };

    hidl_string upstream("invalid");
    Return<void> ret = control->getForwardedStats(upstream, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * GetForwardedStatsDummyIface:
 * Calls getForwardedStats(). Stats should always be 0
 * since there is no data traffic.
 */
TEST_F(OffloadControlHidlTest, GetForwardedStatsDummyIface) {
    auto cb = [&](uint64_t rxBytes, uint64_t txBytes) {
                  EXPECT_EQ((uint64_t) 0, rxBytes);
                  EXPECT_EQ((uint64_t) 0, txBytes);
              };

    hidl_string upstream("dummy0");
    Return<void> ret = control->getForwardedStats(upstream, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetDataLimitEmpty:
 * Calls setDataLimit(). Test queury to upstream with
 * Empty name returns false.
 * Valid parameters are not available since we cannot initialize data.
 */
TEST_F(OffloadControlHidlTest, SetDataLimitEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string upstream("");
    uint64_t limit = 5000;
    Return<void> ret = control->setDataLimit(upstream, limit, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetDataLimit2:
 * Calls setDataLimit(). Test queury to upstream with
 * valid parameter. Should return false since it is not an upstream
 * iface. Cannot init backhaul from testcase.
 */
TEST_F(OffloadControlHidlTest, SetDataLimit2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string upstream("dummy0");
    uint64_t limit = 5000;
    Return<void> ret = control->setDataLimit(upstream, limit, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersSuccess:
 * Calls setUpstreamParameters(). Valid parameters should return
 * true.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersSuccess) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_TRUE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersEmpty:
 * Calls setUpstreamParameters(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("");
    hidl_string v4Addr("");
    hidl_string v4Gw("");
    hidl_vec<hidl_string> v6Gws;
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersIfaceInvalid:
 * Calls setUpstreamParameters(). Invalid iface should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersIfaceInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("invalid");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersIfaceEmpty:
 * Calls setUpstreamParameters(). Empty iface should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersIfaceEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4AddrEmpty: Calls
 * setUpstreamParameters(). Empty v4 address should return
 * false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4AddrInvalid:
 * Calls setUpstreamParameters(). Invalid v4 address should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("invalid");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4AddrInvalid2:
 * Calls setUpstreamParameters(). Invalid v4 address (missing 1 octet)
 * should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4AddrInvalid2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4GwEmpty: Calls
 * setUpstreamParameters(). Empty ipv4 gateway should return
 * false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4GwInvalid:
 * Calls setUpstreamParameters(). Invalid ipv4 gateway should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("invalid");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV4GwInvalid2:
 * Calls setUpstreamParameters(). Invalid v4 gateway (missing 1 octet)
 * should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV4GwInvalid2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8:0:0::1"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV6GwEmpty: Calls
 * setUpstreamParameters(). Invalid IPv6 gateway should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string(""));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV6GwInvalid:
 * Calls setUpstreamParameters(). Invalid IPv6 gateway should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("invalid"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * SetUpstreamParametersV6GwInvalid2:
 * Calls setUpstreamParameters(). Invalid IPv6 gateway (too short) should return false.
 */
TEST_F(OffloadControlHidlTest, SetUpstreamParametersV6GwInvalid2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string v4Addr("8.8.8.8");
    hidl_string v4Gw("8.8.8.1");
    vector<hidl_string> v6Gws;
    v6Gws.push_back(hidl_string("2001:0db8"));
    Return<void> ret = control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstream:
 * Calls addDownstream(). Valid parameters should return true.
 */
TEST_F(OffloadControlHidlTest, AddDownstream) {
    auto cb = [&](bool success, std::string errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };

    hidl_string iface("dummy0");
    hidl_string prefix("8.8.8.8");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstreamEmpty:
 * Calls addDownstream(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("");
    hidl_string prefix("");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstreamEmpty2:
 * Calls addDownstream(). Empty prefix should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamEmpty2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string prefix("");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstreamInvalid:
 * Calls addDownstream(). Invalid parameters should return false.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("invalid");
    hidl_string prefix("8.8.8");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * AddDownstreamInvalid2:
 * Calls addDownstream(). Invalid parameters should return false. Prefix
 * is too short, missing 1 octet.
 */
TEST_F(OffloadControlHidlTest, AddDownstreamInvalid2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string prefix("8.8.8");
    Return<void> ret = control->addDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstream:
 * Calls removeDownstream(). Valid parameters should return true.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstream) {
    auto cb = [&](bool success, std::string errMsg) {
                  if(!success) {
                      ALOGI("Error message: %s", errMsg.c_str());
                  }
                  ASSERT_TRUE(success);
              };

    hidl_string iface("dummy0");
    hidl_string prefix("8.8.8.8");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstreamEmpty:
 * Calls removeDownstream(). Empty parameters should return false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamEmpty) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("");
    hidl_string prefix("");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstreamEmpty2:
 * Calls removeDownstream(). Empty prefix should return
 * false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamEmpty2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string prefix("");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstreamInvalid:
 * Calls removeDownstream(). Invalid parameters should return false.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamInvalid) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("invalid");
    hidl_string prefix("8.8.8");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

/**
 * RemoveDownstreamInvalid2:
 * Calls removeDownstream(). Invalid parameters should return false. Prefix
 * is too short, missing 1 octet.
 */
TEST_F(OffloadControlHidlTest, RemoveDownstreamInvalid2) {
    auto cb = [&](bool success, std::string errMsg) {
                    ALOGI("Error message: %s", errMsg.c_str());
                    EXPECT_FALSE(success);
                };

    hidl_string iface("dummy0");
    hidl_string prefix("8.8.8");
    Return<void> ret = control->removeDownstream(iface, prefix, cb);
    EXPECT_TRUE(ret.isOk());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
