/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "tetheroffload_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tetheroffload/BnOffload.h>
#include <aidl/android/hardware/tetheroffload/BnTetheringOffloadCallback.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <log/log.h>
#include <net/if.h>
#include <sys/socket.h>

namespace aidl::android::hardware::tetheroffload {

namespace {

using ::android::base::unique_fd;
using android::hardware::tetheroffload::ForwardedStats;
using android::hardware::tetheroffload::IOffload;
using android::hardware::tetheroffload::NatTimeoutUpdate;
using android::hardware::tetheroffload::OffloadCallbackEvent;
using ::testing::AnyOf;
using ::testing::Eq;

const std::string TEST_IFACE = "rmnet_data0";
const unsigned kFd1Groups = NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY;
const unsigned kFd2Groups = NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY;

inline const sockaddr* asSockaddr(const sockaddr_nl* nladdr) {
    return reinterpret_cast<const sockaddr*>(nladdr);
}

int netlinkSocket(int protocol, unsigned groups) {
    unique_fd s(socket(AF_NETLINK, SOCK_DGRAM, protocol));
    if (s.get() < 0) {
        return -errno;
    }

    const struct sockaddr_nl bind_addr = {
            .nl_family = AF_NETLINK,
            .nl_pad = 0,
            .nl_pid = 0,
            .nl_groups = groups,
    };
    if (bind(s.get(), asSockaddr(&bind_addr), sizeof(bind_addr)) != 0) {
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

int netlinkSocket(unsigned groups) {
    return netlinkSocket(NETLINK_NETFILTER, groups);
}

// Check whether the specified interface is up.
bool interfaceIsUp(const std::string name) {
    struct ifreq ifr = {};
    strlcpy(ifr.ifr_name, name.c_str(), sizeof(ifr.ifr_name));
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) return false;
    int ret = ioctl(sock, SIOCGIFFLAGS, &ifr, sizeof(ifr));
    close(sock);
    return (ret == 0) && (ifr.ifr_flags & IFF_UP);
}

// Callback class for both events and NAT timeout updates.
class TetheringOffloadCallback : public BnTetheringOffloadCallback {
  public:
    ndk::ScopedAStatus onEvent(OffloadCallbackEvent in_event) override {
        mLastEvent = in_event;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus updateTimeout(const NatTimeoutUpdate& in_params) override {
        mNatTimeout = in_params;
        return ndk::ScopedAStatus::ok();
    }

  private:
    OffloadCallbackEvent mLastEvent;
    NatTimeoutUpdate mNatTimeout;
};

// The common base class for tetheroffload AIDL HAL tests.
class TetheroffloadAidlTestBase : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override { getService(); }
    virtual void TearDown() override{};

  protected:
    void getService() {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mOffload = IOffload::fromBinder(ndk::SpAIBinder(binder));
    }

    void setHandles() {
        unique_fd ufd1(netlinkSocket(kFd1Groups));
        if (ufd1.get() < 0) {
            ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
            FAIL();
        }
        ndk::ScopedFileDescriptor fd1 = ndk::ScopedFileDescriptor(ufd1.release());

        unique_fd ufd2(netlinkSocket(kFd2Groups));
        if (ufd2.get() < 0) {
            ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
            FAIL();
        }
        ndk::ScopedFileDescriptor fd2 = ndk::ScopedFileDescriptor(ufd2.release());

        ASSERT_TRUE(mOffload->setHandles(fd1, fd2).isOk());
    }

    void initOffload(const bool expectedResult) {
        mTetheringOffloadCallback = ndk::SharedRefBase::make<TetheringOffloadCallback>();
        ASSERT_NE(mTetheringOffloadCallback, nullptr) << "Could not get offload callback";
        ASSERT_EQ(mOffload->initOffload(mTetheringOffloadCallback).getExceptionCode(),
                  expectedResult ? EX_NONE : EX_ILLEGAL_STATE);
    }

    void stopOffload(const bool expectedResult) {
        ASSERT_EQ(mOffload->stopOffload().getExceptionCode(),
                  expectedResult ? EX_NONE : EX_ILLEGAL_STATE);
    }

    std::shared_ptr<IOffload> mOffload;
    std::shared_ptr<TetheringOffloadCallback> mTetheringOffloadCallback;
};

// The test class for tetheroffload before initialization.
class TetheroffloadAidlPreInitTest : public TetheroffloadAidlTestBase {
  public:
    virtual void SetUp() override {
        getService();
        setHandles();
    }
};

// The main test class for tetheroffload AIDL HAL.
class TetheroffloadAidlGeneralTest : public TetheroffloadAidlTestBase {
  public:
    virtual void SetUp() override {
        getService();
        setHandles();
        initOffload(true);
    }

    virtual void TearDown() override { stopOffload(true); }
};

// Ensure handles can be set with correct socket options.
TEST_P(TetheroffloadAidlTestBase, TestSetHandles) {
    // Try multiple times in a row to see if it provokes file descriptor leaks.
    for (int i = 0; i < 1024; i++) {
        setHandles();
    }
}

// Passing a handle without an associated file descriptor should return an error
// (e.g. "Failed Input Checks"). Check that this occurs when both FDs are empty.
TEST_P(TetheroffloadAidlTestBase, TestSetHandleNone) {
    ndk::ScopedFileDescriptor fd1 = ndk::ScopedFileDescriptor(-1);
    ndk::ScopedFileDescriptor fd2 = ndk::ScopedFileDescriptor(-1);

    EXPECT_EQ(mOffload->setHandles(fd1, fd2).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Passing a handle without an associated file descriptor should return an error
// (e.g. "Failed Input Checks"). Check that this occurs when FD2 is empty.
TEST_P(TetheroffloadAidlTestBase, TestSetHandle1Only) {
    unique_fd ufd1(netlinkSocket(kFd1Groups));
    if (ufd1.get() < 0) {
        ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
        FAIL();
    }
    ndk::ScopedFileDescriptor fd1 = ndk::ScopedFileDescriptor(ufd1.release());
    ndk::ScopedFileDescriptor fd2 = ndk::ScopedFileDescriptor(-1);

    EXPECT_EQ(mOffload->setHandles(fd1, fd2).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Passing a handle without an associated file descriptor should return an error
// (e.g. "Failed Input Checks"). Check that this occurs when FD1 is empty.
TEST_P(TetheroffloadAidlTestBase, TestSetHandle2OnlyNotOk) {
    ndk::ScopedFileDescriptor fd1 = ndk::ScopedFileDescriptor(-1);
    unique_fd ufd2(netlinkSocket(kFd2Groups));
    if (ufd2.get() < 0) {
        ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
        FAIL();
    }
    ndk::ScopedFileDescriptor fd2 = ndk::ScopedFileDescriptor(ufd2.release());

    EXPECT_EQ(mOffload->setHandles(fd1, fd2).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Call initOffload() multiple times. Check that non-first initOffload() calls return error.
TEST_P(TetheroffloadAidlPreInitTest, AdditionalInitsWithoutStopReturnFalse) {
    initOffload(true);
    initOffload(false);
    initOffload(false);
    initOffload(false);
}

// Check that calling stopOffload() without first having called initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, MultipleStopsWithoutInitReturnFalse) {
    stopOffload(false);
    stopOffload(false);
    stopOffload(false);
}

// Check that calling stopOffload() after a complete init/stop cycle returns error.
TEST_P(TetheroffloadAidlPreInitTest, AdditionalStopsWithInitReturnFalse) {
    initOffload(true);
    // Call setUpstreamParameters() so that "offload" can be reasonably said
    // to be both requested and operational.
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.0.2");
    const std::string v4Gw("192.0.0.1");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1"), std::string("fe80::db8:2")};
    EXPECT_TRUE(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).isOk());
    if (!interfaceIsUp(TEST_IFACE)) {
        return;
    }
    SCOPED_TRACE("Expecting stopOffload to succeed");
    stopOffload(true);  // balance out initOffload(true)
    SCOPED_TRACE("Expecting stopOffload to fail the first time");
    stopOffload(false);
    SCOPED_TRACE("Expecting stopOffload to fail the second time");
    stopOffload(false);
}

// Check that calling setLocalPrefixes() without first having called initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, SetLocalPrefixesWithoutInitReturnsFalse) {
    const std::vector<std::string> prefixes{std::string("2001:db8::/64")};
    EXPECT_EQ(mOffload->setLocalPrefixes(prefixes).getExceptionCode(), EX_ILLEGAL_STATE);
}

// Check that calling getForwardedStats() without first having called initOffload()
// returns zero bytes statistics.
TEST_P(TetheroffloadAidlPreInitTest, GetForwardedStatsWithoutInitReturnsZeroValues) {
    const std::string upstream(TEST_IFACE);
    ForwardedStats stats;
    EXPECT_TRUE(mOffload->getForwardedStats(upstream, &stats).isOk());
    EXPECT_EQ(stats.rxBytes, 0ULL);
    EXPECT_EQ(stats.txBytes, 0ULL);
}

// Check that calling setDataWarningAndLimit() without first having called initOffload() returns
// error.
TEST_P(TetheroffloadAidlPreInitTest, SetDataWarningAndLimitWithoutInitReturnsFalse) {
    const std::string upstream(TEST_IFACE);
    const uint64_t warning = 5000ULL;
    const uint64_t limit = 5000ULL;
    EXPECT_EQ(mOffload->setDataWarningAndLimit(upstream, warning, limit).getExceptionCode(),
              EX_ILLEGAL_STATE);
}

// Check that calling setUpstreamParameters() without first having called initOffload()
// returns error.
TEST_P(TetheroffloadAidlPreInitTest, SetUpstreamParametersWithoutInitReturnsFalse) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.2.0/24");
    const std::string v4Gw("192.0.2.1");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1")};
    EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
              EX_ILLEGAL_STATE);
}

// Check that calling addDownstream() with an IPv4 prefix without first having called
// initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, AddIPv4DownstreamWithoutInitReturnsFalse) {
    const std::string iface(TEST_IFACE);
    const std::string prefix("192.0.2.0/24");
    EXPECT_EQ(mOffload->addDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_STATE);
}

// Check that calling addDownstream() with an IPv6 prefix without first having called
// initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, AddIPv6DownstreamWithoutInitReturnsFalse) {
    const std::string iface(TEST_IFACE);
    const std::string prefix("2001:db8::/64");
    EXPECT_EQ(mOffload->addDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_STATE);
}

// Check that calling removeDownstream() with an IPv4 prefix without first having called
// initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, RemoveIPv4DownstreamWithoutInitReturnsFalse) {
    const std::string iface(TEST_IFACE);
    const std::string prefix("192.0.2.0/24");
    EXPECT_EQ(mOffload->removeDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_STATE);
}

// Check that calling removeDownstream() with an IPv6 prefix without first having called
// initOffload() returns error.
TEST_P(TetheroffloadAidlPreInitTest, RemoveIPv6DownstreamWithoutInitReturnsFalse) {
    const std::string iface(TEST_IFACE);
    const std::string prefix("2001:db8::/64");
    EXPECT_EQ(mOffload->removeDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_STATE);
}

/*
 * Tests for IOffloadControl::setLocalPrefixes().
 */

// Test setLocalPrefixes() accepts an IPv4 address.
TEST_P(TetheroffloadAidlGeneralTest, SetLocalPrefixesIPv4AddressOk) {
    const std::vector<std::string> prefixes{std::string("192.0.2.1")};
    EXPECT_TRUE(mOffload->setLocalPrefixes(prefixes).isOk());
}

// Test setLocalPrefixes() accepts an IPv6 address.
TEST_P(TetheroffloadAidlGeneralTest, SetLocalPrefixesIPv6AddressOk) {
    const std::vector<std::string> prefixes{std::string("fe80::1")};
    EXPECT_TRUE(mOffload->setLocalPrefixes(prefixes).isOk());
}

// Test setLocalPrefixes() accepts both IPv4 and IPv6 prefixes.
TEST_P(TetheroffloadAidlGeneralTest, SetLocalPrefixesIPv4v6PrefixesOk) {
    const std::vector<std::string> prefixes{std::string("192.0.2.0/24"), std::string("fe80::/64")};
    EXPECT_TRUE(mOffload->setLocalPrefixes(prefixes).isOk());
}

// Test that setLocalPrefixes() fails given empty input. There is always
// a non-empty set of local prefixes; when all networking interfaces are down
// we still apply {127.0.0.0/8, ::1/128, fe80::/64} here.
TEST_P(TetheroffloadAidlGeneralTest, SetLocalPrefixesEmptyFails) {
    const std::vector<std::string> prefixes{};
    EXPECT_EQ(mOffload->setLocalPrefixes(prefixes).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Test setLocalPrefixes() fails on incorrectly formed input strings.
TEST_P(TetheroffloadAidlGeneralTest, SetLocalPrefixesInvalidFails) {
    const std::vector<std::string> prefixes{std::string("192.0.2.0/24"), std::string("invalid")};
    EXPECT_EQ(mOffload->setLocalPrefixes(prefixes).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

/*
 * Tests for IOffloadControl::getForwardedStats().
 */

// Test that getForwardedStats() for a non-existent upstream yields zero bytes statistics.
TEST_P(TetheroffloadAidlGeneralTest, GetForwardedStatsInvalidUpstreamIface) {
    const std::string upstream("invalid");
    ForwardedStats stats;
    EXPECT_EQ(mOffload->getForwardedStats(upstream, &stats).getExceptionCode(),
              EX_ILLEGAL_ARGUMENT);
    EXPECT_EQ(stats.rxBytes, 0ULL);
    EXPECT_EQ(stats.txBytes, 0ULL);
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, GetForwardedStatsDummyIface) {
    const std::string upstream(TEST_IFACE);
    ForwardedStats stats;
    EXPECT_TRUE(mOffload->getForwardedStats(upstream, &stats).isOk());
    EXPECT_EQ(stats.rxBytes, 0ULL);
    EXPECT_EQ(stats.txBytes, 0ULL);
}

/*
 * Tests for IOffloadControl::setDataWarningAndLimit().
 */

// Test that setDataWarningAndLimit() for an empty interface name fails.
TEST_P(TetheroffloadAidlGeneralTest, SetDataWarningAndLimitEmptyUpstreamIfaceFails) {
    const std::string upstream("");
    const uint64_t warning = 12345ULL;
    const uint64_t limit = 67890ULL;
    EXPECT_THAT(mOffload->setDataWarningAndLimit(upstream, warning, limit).getExceptionCode(),
                AnyOf(Eq(EX_ILLEGAL_ARGUMENT), Eq(EX_UNSUPPORTED_OPERATION)));
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetDataWarningAndLimitNonZeroOk) {
    const std::string upstream(TEST_IFACE);
    const uint64_t warning = 4000ULL;
    const uint64_t limit = 5000ULL;
    EXPECT_THAT(mOffload->setDataWarningAndLimit(upstream, warning, limit).getExceptionCode(),
                AnyOf(Eq(EX_NONE), Eq(EX_UNSUPPORTED_OPERATION)));
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetDataWarningAndLimitZeroOk) {
    const std::string upstream(TEST_IFACE);
    const uint64_t warning = 0ULL;
    const uint64_t limit = 0ULL;
    EXPECT_THAT(mOffload->setDataWarningAndLimit(upstream, warning, limit).getExceptionCode(),
                AnyOf(Eq(EX_NONE), Eq(EX_UNSUPPORTED_OPERATION)));
}

/*
 * Tests for IOffloadControl::setUpstreamParameters().
 */

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersIPv6OnlyOk) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("");
    const std::string v4Gw("");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1"), std::string("fe80::db8:2")};
    EXPECT_TRUE(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersAlternateIPv6OnlyOk) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr;
    const std::string v4Gw;
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1"), std::string("fe80::db8:3")};
    EXPECT_TRUE(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersIPv4OnlyOk) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.2.2");
    const std::string v4Gw("192.0.2.1");
    const std::vector<std::string> v6Gws{};
    EXPECT_TRUE(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersIPv4v6Ok) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.2.2");
    const std::string v4Gw("192.0.2.1");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1"), std::string("fe80::db8:2")};
    EXPECT_TRUE(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).isOk());
}

// Test that setUpstreamParameters() fails when all parameters are empty.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersEmptyFails) {
    const std::string upstream("");
    const std::string v4Addr("");
    const std::string v4Gw("");
    const std::vector<std::string> v6Gws{};
    EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
              EX_ILLEGAL_ARGUMENT);
}

// Test that setUpstreamParameters() fails when given empty or non-existent interface names.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersBogusIfaceFails) {
    const std::string v4Addr("192.0.2.2");
    const std::string v4Gw("192.0.2.1");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1")};
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(testing::Message() << "upstream: " << bogus);
        const std::string upstream(bogus);
        EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv4 addresses.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersInvalidIPv4AddrFails) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Gw("192.0.2.1");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1")};
    for (const auto& bogus : {"invalid", "192.0.2"}) {
        SCOPED_TRACE(testing::Message() << "v4addr: " << bogus);
        const std::string v4Addr(bogus);
        EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv4 gateways.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersInvalidIPv4GatewayFails) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.2.2");
    const std::vector<std::string> v6Gws{std::string("fe80::db8:1")};
    for (const auto& bogus : {"invalid", "192.0.2"}) {
        SCOPED_TRACE(testing::Message() << "v4gateway: " << bogus);
        const std::string v4Gw(bogus);
        EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv6 gateways.
TEST_P(TetheroffloadAidlGeneralTest, SetUpstreamParametersBadIPv6GatewaysFail) {
    const std::string upstream(TEST_IFACE);
    const std::string v4Addr("192.0.2.2");
    const std::string v4Gw("192.0.2.1");
    for (const auto& bogus : {"", "invalid", "fe80::bogus", "192.0.2.66"}) {
        SCOPED_TRACE(testing::Message() << "v6gateway: " << bogus);
        const std::vector<std::string> v6Gws{std::string("fe80::1"), std::string(bogus)};
        EXPECT_EQ(mOffload->setUpstreamParameters(upstream, v4Addr, v4Gw, v6Gws).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

/*
 * Tests for IOffloadControl::addDownstream().
 */

// Test addDownstream() works given an IPv4 prefix.
TEST_P(TetheroffloadAidlGeneralTest, AddDownstreamIPv4) {
    const std::string iface("dummy0");
    const std::string prefix("192.0.2.0/24");
    EXPECT_TRUE(mOffload->addDownstream(iface, prefix).isOk());
}

// Test addDownstream() works given an IPv6 prefix.
TEST_P(TetheroffloadAidlGeneralTest, AddDownstreamIPv6) {
    const std::string iface("dummy0");
    const std::string prefix("2001:db8::/64");
    EXPECT_TRUE(mOffload->addDownstream(iface, prefix).isOk());
}

// Test addDownstream() fails given all empty parameters.
TEST_P(TetheroffloadAidlGeneralTest, AddDownstreamEmptyFails) {
    const std::string iface("");
    const std::string prefix("");
    EXPECT_EQ(mOffload->addDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Test addDownstream() fails given empty or non-existent interface names.
TEST_P(TetheroffloadAidlGeneralTest, AddDownstreamInvalidIfaceFails) {
    const std::string prefix("192.0.2.0/24");
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(testing::Message() << "iface: " << bogus);
        const std::string iface(bogus);
        EXPECT_EQ(mOffload->addDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
    }
}

// Test addDownstream() fails given unparseable prefix arguments.
TEST_P(TetheroffloadAidlGeneralTest, AddDownstreamBogusPrefixFails) {
    const std::string iface("dummy0");
    for (const auto& bogus : {"", "192.0.2/24", "2001:db8/64"}) {
        SCOPED_TRACE(testing::Message() << "prefix: " << bogus);
        const std::string prefix(bogus);
        EXPECT_EQ(mOffload->addDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
    }
}

/*
 * Tests for IOffloadControl::removeDownstream().
 */

// Test removeDownstream() works given an IPv4 prefix.
TEST_P(TetheroffloadAidlGeneralTest, RemoveDownstreamIPv4) {
    const std::string iface("dummy0");
    const std::string prefix("192.0.2.0/24");
    // First add the downstream, otherwise removeDownstream logic can reasonably
    // return error for downstreams not previously added.
    EXPECT_TRUE(mOffload->addDownstream(iface, prefix).isOk());
    EXPECT_TRUE(mOffload->removeDownstream(iface, prefix).isOk());
}

// Test removeDownstream() works given an IPv6 prefix.
TEST_P(TetheroffloadAidlGeneralTest, RemoveDownstreamIPv6) {
    const std::string iface("dummy0");
    const std::string prefix("2001:db8::/64");
    // First add the downstream, otherwise removeDownstream logic can reasonably
    // return error for downstreams not previously added.
    EXPECT_TRUE(mOffload->addDownstream(iface, prefix).isOk());
    EXPECT_TRUE(mOffload->removeDownstream(iface, prefix).isOk());
}

// Test removeDownstream() fails given all empty parameters.
TEST_P(TetheroffloadAidlGeneralTest, RemoveDownstreamEmptyFails) {
    const std::string iface("");
    const std::string prefix("");
    EXPECT_EQ(mOffload->removeDownstream(iface, prefix).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// Test removeDownstream() fails given empty or non-existent interface names.
TEST_P(TetheroffloadAidlGeneralTest, RemoveDownstreamBogusIfaceFails) {
    const std::string prefix("192.0.2.0/24");
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(testing::Message() << "iface: " << bogus);
        const std::string iface(bogus);
        EXPECT_EQ(mOffload->removeDownstream(iface, prefix).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

// Test removeDownstream() fails given unparseable prefix arguments.
TEST_P(TetheroffloadAidlGeneralTest, RemoveDownstreamBogusPrefixFails) {
    const std::string iface("dummy0");
    for (const auto& bogus : {"", "192.0.2/24", "2001:db8/64"}) {
        SCOPED_TRACE(testing::Message() << "prefix: " << bogus);
        const std::string prefix(bogus);
        EXPECT_EQ(mOffload->removeDownstream(iface, prefix).getExceptionCode(),
                  EX_ILLEGAL_ARGUMENT);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TetheroffloadAidlTestBase);
INSTANTIATE_TEST_SUITE_P(
        IOffload, TetheroffloadAidlTestBase,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IOffload::descriptor)),
        ::android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TetheroffloadAidlPreInitTest);
INSTANTIATE_TEST_SUITE_P(
        IOffload, TetheroffloadAidlPreInitTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IOffload::descriptor)),
        ::android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TetheroffloadAidlGeneralTest);
INSTANTIATE_TEST_SUITE_P(
        IOffload, TetheroffloadAidlGeneralTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IOffload::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}

}  // namespace aidl::android::hardware::tetheroffload
