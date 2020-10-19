/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "VtsOffloadControlV1_1TargetTest"

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>
#include <android/hardware/tetheroffload/control/1.1/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netlink.h>
#include <log/log.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <set>

using android::sp;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair;
using android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate;
using android::hardware::tetheroffload::control::V1_0::NetworkProtocol;
using android::hardware::tetheroffload::control::V1_0::OffloadCallbackEvent;
using android::hardware::tetheroffload::control::V1_1::IOffloadControl;

enum class ExpectBoolean {
    Ignored = -1,
    False = 0,
    True = 1,
};

constexpr const char* TEST_IFACE = "rmnet_data0";

// We use #defines here so as to get local lamba captures and error message line numbers
#define ASSERT_TRUE_CALLBACK                                                    \
    [&](bool success, std::string errMsg) {                                     \
        std::string msg = StringPrintf("unexpected error: %s", errMsg.c_str()); \
        ASSERT_TRUE(success) << msg;                                            \
    }

#define ASSERT_FALSE_CALLBACK                                                 \
    [&](bool success, std::string errMsg) {                                   \
        std::string msg = StringPrintf("expected error: %s", errMsg.c_str()); \
        ASSERT_FALSE(success) << msg;                                         \
    }

#define ASSERT_ZERO_BYTES_CALLBACK            \
    [&](uint64_t rxBytes, uint64_t txBytes) { \
        EXPECT_EQ(0ULL, rxBytes);             \
        EXPECT_EQ(0ULL, txBytes);             \
    }

inline const sockaddr* asSockaddr(const sockaddr_nl* nladdr) {
    return reinterpret_cast<const sockaddr*>(nladdr);
}

int conntrackSocket(unsigned groups) {
    unique_fd s(socket(AF_NETLINK, SOCK_DGRAM, NETLINK_NETFILTER));
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

class OffloadControlHidlTestBase
    : public testing::TestWithParam<std::tuple<std::string, std::string>> {
  public:
    virtual void SetUp() override {
        setupConfigHal();
        prepareControlHal();
    }

    virtual void TearDown() override {
        // For good measure, we should try stopOffload() once more. Since we
        // don't know where we are in HAL call test cycle we don't know what
        // return code to actually expect, so we just ignore it.
        stopOffload(ExpectBoolean::Ignored);
    }

    // The IOffloadConfig HAL is tested more thoroughly elsewhere. He we just
    // setup everything correctly and verify basic readiness.
    void setupConfigHal() {
        config = IOffloadConfig::getService(std::get<0>(GetParam()));
        ASSERT_NE(nullptr, config.get()) << "Could not get HIDL instance";

        unique_fd fd1(conntrackSocket(NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY));
        if (fd1.get() < 0) {
            ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
            FAIL();
        }
        native_handle_t* const nativeHandle1 = native_handle_create(1, 0);
        nativeHandle1->data[0] = fd1.release();
        hidl_handle h1;
        h1.setTo(nativeHandle1, true);

        unique_fd fd2(conntrackSocket(NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY));
        if (fd2.get() < 0) {
            ALOGE("Unable to create conntrack handles: %d/%s", errno, strerror(errno));
            FAIL();
        }
        native_handle_t* const nativeHandle2 = native_handle_create(1, 0);
        nativeHandle2->data[0] = fd2.release();
        hidl_handle h2;
        h2.setTo(nativeHandle2, true);

        const Return<void> ret = config->setHandles(h1, h2, ASSERT_TRUE_CALLBACK);
        ASSERT_TRUE(ret.isOk());
    }

    void prepareControlHal() {
        control = IOffloadControl::getService(std::get<1>(GetParam()));
        ASSERT_NE(nullptr, control.get()) << "Could not get HIDL instance";

        control_cb = new TetheringOffloadCallback();
        ASSERT_NE(nullptr, control_cb.get()) << "Could not get get offload callback";
    }

    void initOffload(const bool expected_result) {
        auto init_cb = [&](bool success, std::string errMsg) {
            std::string msg = StringPrintf("Unexpectedly %s to init offload: %s",
                                           success ? "succeeded" : "failed", errMsg.c_str());
            ASSERT_EQ(expected_result, success) << msg;
        };
        const Return<void> ret = control->initOffload(control_cb, init_cb);
        ASSERT_TRUE(ret.isOk());
    }

    void setupControlHal() {
        prepareControlHal();
        initOffload(true);
    }

    void stopOffload(const ExpectBoolean value) {
        auto cb = [&](bool success, const hidl_string& errMsg) {
            switch (value) {
                case ExpectBoolean::False:
                    ASSERT_EQ(false, success) << "Unexpectedly able to stop offload: " << errMsg;
                    break;
                case ExpectBoolean::True:
                    ASSERT_EQ(true, success) << "Unexpectedly failed to stop offload: " << errMsg;
                    break;
                case ExpectBoolean::Ignored:
                    break;
            }
        };
        const Return<void> ret = control->stopOffload(cb);
        ASSERT_TRUE(ret.isOk());
    }

    // Callback class for both events and NAT timeout updates.
    class TetheringOffloadCallback
        : public testing::VtsHalHidlTargetCallbackBase<TetheringOffloadCallbackArgs>,
          public ITetheringOffloadCallback {
      public:
        TetheringOffloadCallback() = default;
        virtual ~TetheringOffloadCallback() = default;

        Return<void> onEvent(OffloadCallbackEvent event) override {
            const TetheringOffloadCallbackArgs args{.last_event = event};
            NotifyFromCallback(kCallbackOnEvent, args);
            return Void();
        };

        Return<void> updateTimeout(const NatTimeoutUpdate& params) override {
            const TetheringOffloadCallbackArgs args{.last_params = params};
            NotifyFromCallback(kCallbackUpdateTimeout, args);
            return Void();
        };
    };

    sp<IOffloadConfig> config;
    sp<IOffloadControl> control;
    sp<TetheringOffloadCallback> control_cb;
};

// Check that calling setDataWarningAndLimit() without first having called initOffload() returns
// false.
TEST_P(OffloadControlHidlTestBase, SetDataWarningAndLimitWithoutInitReturnsFalse) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret =
            control->setDataWarningAndLimit(upstream, 5000ULL, 5000ULL, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

class OffloadControlHidlTest : public OffloadControlHidlTestBase {
  public:
    virtual void SetUp() override {
        setupConfigHal();
        setupControlHal();
    }

    virtual void TearDown() override {
        // For good measure, we should try stopOffload() once more. Since we
        // don't know where we are in HAL call test cycle we don't know what
        // return code to actually expect, so we just ignore it.
        stopOffload(ExpectBoolean::Ignored);
    }
};

/*
 * Tests for IOffloadControl::setDataWarningAndLimit().
 */

// Test that setDataWarningAndLimit() for an empty interface name fails.
TEST_P(OffloadControlHidlTest, SetDataWarningAndLimitEmptyUpstreamIfaceFails) {
    const hidl_string upstream("");
    const Return<void> ret =
            control->setDataWarningAndLimit(upstream, 12345ULL, 67890ULL, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlHidlTest, SetDataWarningAndLimitNonZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret1 =
            control->setDataWarningAndLimit(upstream, 4000ULL, 5000ULL, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret1.isOk());
    // Verify warning greater than limit is also accepted by hardware.
    const Return<void> ret2 =
            control->setDataWarningAndLimit(upstream, 5000ULL, 4000ULL, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret2.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlHidlTest, SetDataWarningAndLimitZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret =
            control->setDataWarningAndLimit(upstream, 0ULL, 0ULL, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlHidlTestBase,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlHidlTest,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
