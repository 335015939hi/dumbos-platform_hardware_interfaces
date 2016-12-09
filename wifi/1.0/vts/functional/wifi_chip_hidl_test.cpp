/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include <android/hardware/wifi/1.0/IWifiChip.h>

#include <gtest/gtest.h>

#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::ChipId;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferVerboseLevel;
using ::android::hardware::wifi::V1_0::WifiDebugHostWakeReasonStats;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiApIface;
using ::android::hardware::wifi::V1_0::IWifiIface;
using ::android::hardware::wifi::V1_0::IWifiNanIface;
using ::android::hardware::wifi::V1_0::IWifiP2pIface;
using ::android::hardware::wifi::V1_0::IWifiRttController;
using ::android::hardware::wifi::V1_0::IWifiStaIface;

namespace {
constexpr WifiDebugRingBufferVerboseLevel kDebugRingBufferVerboseLvl =
    WifiDebugRingBufferVerboseLevel::VERBOSE;
constexpr uint32_t kDebugRingBufferMaxInterval = 5;
constexpr uint32_t kDebugRingBufferMaxDataSize = 1024;

/**
 * Check if any of the ring buffer capabilities are set.
 */
bool hasAnyRingBufferCapabilities(uint32_t caps) {
    return (caps &
            (IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_CONNECT_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_POWER_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_WAKELOCK_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_VENDOR_DATA));
}
}  // namespace

/**
 * Fixture to use for all Wifi chip HIDL interface tests.
 */
class WifiChipHidlTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        wifi_chip_ = getWifiChip();
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    virtual void TearDown() override { stopWifi(); }

   protected:
    // Helper function to configure the Chip in one of the supported modes.
    // Most of the non-mode-configuration-related methods require chip
    // to be first configured.
    ChipModeId configureChipForIfaceType(IfaceType type) {
        ChipModeId mode_id;
        EXPECT_TRUE(
            configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    uint32_t configureChipForStaIfaceAndGetCapabilities() {
        configureChipForIfaceType(IfaceType::STA);
        uint32_t chip_caps;
        wifi_chip_->getCapabilities(
            [&](const WifiStatus& status, uint32_t caps) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
                chip_caps = caps;
            });
        return chip_caps;
    }

    std::string getIfaceName(const sp<IWifiIface>& iface) {
        std::string iface_name;
        iface->getName([&](const WifiStatus& status, const hidl_string& name) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            iface_name = name.c_str();
        });
        return iface_name;
    }

    WifiStatusCode createApIface(sp<IWifiApIface>* ap_iface) {
        WifiStatus wifi_status;
        wifi_chip_->createApIface(
            [&](const WifiStatus& status, const sp<IWifiApIface>& iface) {
                wifi_status = status;
                *ap_iface = iface;
            });
        return wifi_status.code;
    }

    WifiStatusCode removeApIface(const std::string& name) {
        WifiStatus wifi_status;
        wifi_chip_->removeApIface(
            name, [&](const WifiStatus& status) { wifi_status = status; });
        return wifi_status.code;
    }

    WifiStatusCode createNanIface(sp<IWifiNanIface>* nan_iface) {
        WifiStatus wifi_status;
        wifi_chip_->createNanIface(
            [&](const WifiStatus& status, const sp<IWifiNanIface>& iface) {
                wifi_status = status;
                *nan_iface = iface;
            });
        return wifi_status.code;
    }

    WifiStatusCode removeNanIface(const std::string& name) {
        WifiStatus wifi_status;
        wifi_chip_->removeNanIface(
            name, [&](const WifiStatus& status) { wifi_status = status; });
        return wifi_status.code;
    }

    WifiStatusCode createP2pIface(sp<IWifiP2pIface>* p2p_iface) {
        WifiStatus wifi_status;
        wifi_chip_->createP2pIface(
            [&](const WifiStatus& status, const sp<IWifiP2pIface>& iface) {
                wifi_status = status;
                *p2p_iface = iface;
            });
        return wifi_status.code;
    }

    WifiStatusCode removeP2pIface(const std::string& name) {
        WifiStatus wifi_status;
        wifi_chip_->removeP2pIface(
            name, [&](const WifiStatus& status) { wifi_status = status; });
        return wifi_status.code;
    }

    WifiStatusCode createStaIface(sp<IWifiStaIface>* sta_iface) {
        WifiStatus wifi_status;
        wifi_chip_->createStaIface(
            [&](const WifiStatus& status, const sp<IWifiStaIface>& iface) {
                wifi_status = status;
                *sta_iface = iface;
            });
        return wifi_status.code;
    }

    WifiStatusCode removeStaIface(const std::string& name) {
        WifiStatus wifi_status;
        wifi_chip_->removeStaIface(
            name, [&](const WifiStatus& status) { wifi_status = status; });
        return wifi_status.code;
    }

    sp<IWifiChip> wifi_chip_;
};

/*
 * Create:
 * Ensures that an instance of the IWifiChip proxy object is
 * successfully created.
 */
TEST(WifiChipHidlTestNoFixture, Create) {
    EXPECT_NE(nullptr, getWifiChip().get());
    stopWifi();
}

/*
 * GetId:
 */
TEST_F(WifiChipHidlTest, GetId) {
    wifi_chip_->getId([&](const WifiStatus& status, ChipId /* id */) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    });
}

/*
 * GetAvailableMode:
 */
TEST_F(WifiChipHidlTest, GetAvailableModes) {
    wifi_chip_->getAvailableModes([&](
        const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_LT(0u, modes.size());
    });
}

/*
 * ConfigureChip:
 */
TEST_F(WifiChipHidlTest, ConfigureChip) {
    std::vector<IWifiChip::ChipMode> chip_modes;
    wifi_chip_->getAvailableModes([&](
        const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_LT(0u, modes.size());
        chip_modes = modes;
    });
    for (const auto& mode : chip_modes) {
        wifi_chip_->configureChip(mode.id, [&](const WifiStatus& status) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
    }
}

/*
 * GetCapabilities:
 */
TEST_F(WifiChipHidlTest, GetCapabilities) {
    configureChipForIfaceType(IfaceType::STA);
    wifi_chip_->getCapabilities([&](const WifiStatus& status, uint32_t caps) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_NE(0u, caps);
    });
}

/*
 * GetMode:
 */
TEST_F(WifiChipHidlTest, GetMode) {
    ChipModeId chip_mode_id = configureChipForIfaceType(IfaceType::STA);
    wifi_chip_->getMode([&](const WifiStatus& status, ChipModeId mode_id) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_EQ(chip_mode_id, mode_id);
    });
}

/*
 * RequestChipDebugInfo:
 */
TEST_F(WifiChipHidlTest, RequestChipDebugInfo) {
    configureChipForStaIfaceAndGetCapabilities();
    wifi_chip_->requestChipDebugInfo(
        [&](const WifiStatus& status, const IWifiChip::ChipDebugInfo& info) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_LT(0u, info.driverDescription.size());
            EXPECT_LT(0u, info.firmwareDescription.size());
        });
}

/*
 * RequestFirmwareDebugDump
 */
TEST_F(WifiChipHidlTest, RequestFirmwareDebugDump) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    wifi_chip_->requestFirmwareDebugDump([&](
        const WifiStatus& status, const hidl_vec<uint8_t>& /* dump */) {
        if (caps & IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_FIRMWARE_DUMP) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        } else {
            EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
        }
    });
}

/*
 * RequestDriverDebugDump
 */
TEST_F(WifiChipHidlTest, RequestDriverDebugDump) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    wifi_chip_->requestDriverDebugDump([&](
        const WifiStatus& status, const hidl_vec<uint8_t>& /* dump */) {
        if (caps & IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_DRIVER_DUMP) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        } else {
            EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
        }
    });
}

/*
 * GetDebugRingBuffersStatus
 */
TEST_F(WifiChipHidlTest, GetDebugRingBuffersStatus) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    wifi_chip_->getDebugRingBuffersStatus(
        [&](const WifiStatus& status,
            const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
            if (hasAnyRingBufferCapabilities(caps)) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
                for (const auto& ring_buffer : ring_buffers) {
                    EXPECT_GT(0u, ring_buffer.ringName.size());
                }
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
}

/*
 * StartLoggingToDebugRingBuffer
 */
TEST_F(WifiChipHidlTest, StartLoggingToDebugRingBuffer) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    std::string ring_name;
    wifi_chip_->getDebugRingBuffersStatus(
        [&](const WifiStatus& status,
            const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
            if (hasAnyRingBufferCapabilities(caps)) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
                ASSERT_GT(0u, ring_buffers.size());
                ring_name = ring_buffers[0].ringName.c_str();
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
    wifi_chip_->startLoggingToDebugRingBuffer(
        ring_name, kDebugRingBufferVerboseLvl, kDebugRingBufferMaxInterval,
        kDebugRingBufferMaxDataSize, [&](const WifiStatus& status) {
            if (hasAnyRingBufferCapabilities(caps)) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
}

/*
 * ForceDumpToDebugRingBuffer
 */
TEST_F(WifiChipHidlTest, ForceDumpToDebugRingBuffer) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    std::string ring_name;
    wifi_chip_->getDebugRingBuffersStatus(
        [&](const WifiStatus& status,
            const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
            if (hasAnyRingBufferCapabilities(caps)) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
                ASSERT_GT(0u, ring_buffers.size());
                ring_name = ring_buffers[0].ringName.c_str();
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
    wifi_chip_->forceDumpToDebugRingBuffer(
        ring_name, [&](const WifiStatus& status) {
            if (hasAnyRingBufferCapabilities(caps)) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
}

/*
 * GetDebugHostWakeReasonStats
 */
TEST_F(WifiChipHidlTest, GetDebugHostWakeReasonStats) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    wifi_chip_->getDebugHostWakeReasonStats(
        [&](const WifiStatus& status,
            const WifiDebugHostWakeReasonStats& /*  stats */) {
            if (caps &
                IWifiChip::ChipCapabilityMask::DEBUG_HOST_WAKE_REASON_STATS) {
                EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            } else {
                EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
            }
        });
}

/*
 * CreateApIface
 * Configures the chip in AP mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateApIface) {
    configureChipForIfaceType(IfaceType::AP);

    sp<IWifiApIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    EXPECT_EQ(WifiStatusCode::ERROR_NOT_AVAILABLE, createApIface(&iface));
}

/*
 * GetApIfaceNames
 * Configures the chip in AP mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetApIfaceNames) {
    configureChipForIfaceType(IfaceType::AP);

    wifi_chip_->getApIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });

    sp<IWifiApIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    wifi_chip_->getApIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(1u, names.size());
            EXPECT_EQ(names[0], iface_name);
        });

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeApIface(iface_name));
    wifi_chip_->getApIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });
}

/*
 * GetApIface
 * Configures the chip in AP mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetApIface) {
    configureChipForIfaceType(IfaceType::AP);

    sp<IWifiApIface> ap_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&ap_iface));
    EXPECT_NE(nullptr, ap_iface.get());

    std::string iface_name = getIfaceName(ap_iface);
    wifi_chip_->getApIface(iface_name, [&](const WifiStatus& status,
                                           const sp<IWifiApIface>& iface) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_NE(nullptr, iface.get());
    });

    std::string invalid_name = iface_name + "0";
    wifi_chip_->getApIface(invalid_name, [&](const WifiStatus& status,
                                             const sp<IWifiApIface>& iface) {
        EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status.code);
        EXPECT_EQ(nullptr, iface.get());
    });
}

/*
 * RemoveApIface
 * Configures the chip in AP mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveApIface) {
    configureChipForIfaceType(IfaceType::AP);

    sp<IWifiApIface> ap_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&ap_iface));
    EXPECT_NE(nullptr, ap_iface.get());

    std::string iface_name = getIfaceName(ap_iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeApIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeApIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeApIface(iface_name));
}

/*
 * CreateNanIface
 * Configures the chip in NAN mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateNanIface) {
    configureChipForIfaceType(IfaceType::NAN);

    sp<IWifiNanIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createNanIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    EXPECT_EQ(WifiStatusCode::ERROR_NOT_AVAILABLE, createNanIface(&iface));
}

/*
 * GetNanIfaceNames
 * Configures the chip in NAN mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetNanIfaceNames) {
    configureChipForIfaceType(IfaceType::NAN);

    wifi_chip_->getNanIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });

    sp<IWifiNanIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createNanIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    wifi_chip_->getNanIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(1u, names.size());
            EXPECT_EQ(names[0], iface_name);
        });

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeNanIface(iface_name));
    wifi_chip_->getNanIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });
}

/*
 * GetNanIface
 * Configures the chip in NAN mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetNanIface) {
    configureChipForIfaceType(IfaceType::NAN);

    sp<IWifiNanIface> nan_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createNanIface(&nan_iface));
    EXPECT_NE(nullptr, nan_iface.get());

    std::string iface_name = getIfaceName(nan_iface);
    wifi_chip_->getNanIface(iface_name, [&](const WifiStatus& status,
                                            const sp<IWifiNanIface>& iface) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_NE(nullptr, iface.get());
    });

    std::string invalid_name = iface_name + "0";
    wifi_chip_->getNanIface(invalid_name, [&](const WifiStatus& status,
                                              const sp<IWifiNanIface>& iface) {
        EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status.code);
        EXPECT_EQ(nullptr, iface.get());
    });
}

/*
 * RemoveNanIface
 * Configures the chip in NAN mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveNanIface) {
    configureChipForIfaceType(IfaceType::NAN);

    sp<IWifiNanIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createNanIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeNanIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeNanIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeNanIface(iface_name));
}

/*
 * CreateP2pIface
 * Configures the chip in P2P mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateP2pIface) {
    configureChipForIfaceType(IfaceType::P2P);

    sp<IWifiP2pIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    EXPECT_EQ(WifiStatusCode::ERROR_NOT_AVAILABLE, createP2pIface(&iface));
}

/*
 * GetP2pIfaceNames
 * Configures the chip in P2P mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetP2pIfaceNames) {
    configureChipForIfaceType(IfaceType::P2P);

    wifi_chip_->getP2pIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });

    sp<IWifiP2pIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    wifi_chip_->getP2pIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(1u, names.size());
            EXPECT_EQ(names[0], iface_name);
        });

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeP2pIface(iface_name));
    wifi_chip_->getP2pIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });
}

/*
 * GetP2pIface
 * Configures the chip in P2P mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetP2pIface) {
    configureChipForIfaceType(IfaceType::P2P);

    sp<IWifiP2pIface> p2p_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&p2p_iface));
    EXPECT_NE(nullptr, p2p_iface.get());

    std::string iface_name = getIfaceName(p2p_iface);
    wifi_chip_->getP2pIface(iface_name, [&](const WifiStatus& status,
                                            const sp<IWifiP2pIface>& iface) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_NE(nullptr, iface.get());
    });

    std::string invalid_name = iface_name + "0";
    wifi_chip_->getP2pIface(invalid_name, [&](const WifiStatus& status,
                                              const sp<IWifiP2pIface>& iface) {
        EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status.code);
        EXPECT_EQ(nullptr, iface.get());
    });
}

/*
 * RemoveP2pIface
 * Configures the chip in P2P mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveP2pIface) {
    configureChipForIfaceType(IfaceType::P2P);

    sp<IWifiP2pIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeP2pIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeP2pIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeP2pIface(iface_name));
}

/*
 * CreateStaIface
 * Configures the chip in STA mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateStaIface) {
    configureChipForIfaceType(IfaceType::STA);

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    EXPECT_EQ(WifiStatusCode::ERROR_NOT_AVAILABLE, createStaIface(&iface));
}

/*
 * GetStaIfaceNames
 * Configures the chip in STA mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetStaIfaceNames) {
    configureChipForIfaceType(IfaceType::STA);

    wifi_chip_->getStaIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    wifi_chip_->getStaIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(1u, names.size());
            EXPECT_EQ(names[0], iface_name);
        });

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeStaIface(iface_name));
    wifi_chip_->getStaIfaceNames(
        [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_EQ(0u, names.size());
        });
}

/*
 * GetStaIface
 * Configures the chip in STA mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetStaIface) {
    configureChipForIfaceType(IfaceType::STA);

    sp<IWifiStaIface> sta_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&sta_iface));
    EXPECT_NE(nullptr, sta_iface.get());

    std::string iface_name = getIfaceName(sta_iface);
    wifi_chip_->getStaIface(iface_name, [&](const WifiStatus& status,
                                            const sp<IWifiStaIface>& iface) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
        EXPECT_NE(nullptr, iface.get());
    });

    std::string invalid_name = iface_name + "0";
    wifi_chip_->getStaIface(invalid_name, [&](const WifiStatus& status,
                                              const sp<IWifiStaIface>& iface) {
        EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status.code);
        EXPECT_EQ(nullptr, iface.get());
    });
}

/*
 * RemoveStaIface
 * Configures the chip in STA mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveStaIface) {
    configureChipForIfaceType(IfaceType::STA);

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeStaIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeStaIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeStaIface(iface_name));
}

/*
 * CreateRttController
 */
TEST_F(WifiChipHidlTest, CreateRttController) {
    configureChipForIfaceType(IfaceType::AP);

    sp<IWifiApIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    sp<IWifiRttController> wifi_rtt_controller;
    wifi_chip_->createRttController(
        iface, [&](const WifiStatus& status,
                   const sp<IWifiRttController>& rtt_controller) {
            EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
            EXPECT_NE(nullptr, rtt_controller.get());
        });
}
