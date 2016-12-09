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
constexpr uint32_t kDebugRingBufferCapabilityMask =
    (IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_CONNECT_EVENT_SUPPORTED |
     IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_POWER_EVENT_SUPPORTED |
     IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_WAKELOCK_EVENT_SUPPORTED |
     IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_VENDOR_DATA_SUPPORTED);
constexpr WifiDebugRingBufferVerboseLevel kDebugRingBufferVerboseLvl =
    WifiDebugRingBufferVerboseLevel::VERBOSE;
constexpr uint32_t kDebugRingBufferMaxInterval = 5;
constexpr uint32_t kDebugRingBufferMaxDataSize = 1024;
}  // namespace

class WifiChipHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    wifi_chip_ = getWifiChip();
    ASSERT_NE(wifi_chip_, nullptr);
  }

  virtual void TearDown() override { stopWifi(); }

 protected:
  // Helper function to configure the Chip in one of the supported modes.
  // Most of the non mode configuration related methods require chip
  // to be first configured.
  ChipModeId configureChip(IfaceType type) {
    ChipModeId mode_id;
    EXPECT_TRUE(configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
    return mode_id;
  }

  uint32_t configureChipAndGetCapabilities() {
    configureChip(IfaceType::STA);
    uint32_t chip_caps;
    wifi_chip_->getCapabilities([&](const WifiStatus& status, uint32_t caps) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
      chip_caps = caps;
    });
    return chip_caps;
  }

  std::string getIfaceName(const sp<IWifiIface>& iface) {
    std::string iface_name;
    iface->getName([&](const WifiStatus& status, const hidl_string& name) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
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
  EXPECT_NE(getWifiChip(), nullptr);
  stopWifi();
}

/*
 * GetId:
 * Invokes the |IWifiChip.getId| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetId) {
  wifi_chip_->getId([&](const WifiStatus& status, ChipId id) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_EQ(id, 0u);
  });
}

/*
 * GetAvailableMode:
 * Invokes the |IWifiChip.getAvailableModes| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetAvailableModes) {
  wifi_chip_->getAvailableModes([&](
      const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_GT(modes.size(), 0u);
  });
}

/*
 * ConfigureChip:
 * Invokes the |IWifiChip.configureChip| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, ConfigureChip) {
  std::vector<IWifiChip::ChipMode> chip_modes;
  wifi_chip_->getAvailableModes([&](
      const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_GT(modes.size(), 0u);
    chip_modes = modes;
  });
  for (const auto& mode : chip_modes) {
    wifi_chip_->configureChip(mode.id, [&](const WifiStatus& status) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    });
  }
}

/*
 * GetCapabilities:
 * Invokes the |IWifiChip.getCapabilities| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetCapabilities) {
  configureChip(IfaceType::STA);
  wifi_chip_->getCapabilities([&](const WifiStatus& status, uint32_t caps) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_NE(caps, 0u);
  });
}

/*
 * GetMode:
 * Invokes the |IWifiChip.getMode| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetMode) {
  ChipModeId chip_mode_id = configureChip(IfaceType::STA);
  wifi_chip_->getMode([&](const WifiStatus& status, ChipModeId mode_id) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_EQ(mode_id, chip_mode_id);
  });
}

/*
 * RequestChipDebugInfo:
 * Invokes the |IWifiChip.requestChipDebugInfo| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, RequestChipDebugInfo) {
  configureChipAndGetCapabilities();
  wifi_chip_->requestChipDebugInfo(
      [&](const WifiStatus& status, const IWifiChip::ChipDebugInfo& info) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_GT(info.driverDescription.size(), 0u);
        EXPECT_GT(info.firmwareDescription.size(), 0u);
      });
}

/*
 * RequestFirmwareDebugDump
 * Invokes the |IWifiChip.requestFirmwareDebugDump| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, RequestFirmwareDebugDump) {
  uint32_t caps = configureChipAndGetCapabilities();
  wifi_chip_->requestFirmwareDebugDump([&](const WifiStatus& status,
                                           const hidl_vec<uint8_t>& dump) {
    if (caps &
        IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_FIRMWARE_DUMP_SUPPORTED) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
      //EXPECT_GT(dump.size(), 0u);
    } else {
      EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
    }
  });
}

/*
 * RequestDriverDebugDump
 * Invokes the |IWifiChip.requestDriverDebugDump| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, RequestDriverDebugDump) {
  uint32_t caps = configureChipAndGetCapabilities();
  wifi_chip_->requestDriverDebugDump(
      [&](const WifiStatus& status, const hidl_vec<uint8_t>& dump) {
        if (caps &
            IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_DRIVER_DUMP_SUPPORTED) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
          //EXPECT_GT(dump.size(), 0u);
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
}

/*
 * GetDebugRingBuffersStatus
 * Invokes the |IWifiChip.getDebugRingBuffersStatus| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetDebugRingBuffersStatus) {
  uint32_t caps = configureChipAndGetCapabilities();
  wifi_chip_->getDebugRingBuffersStatus(
      [&](const WifiStatus& status,
          const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
        if (caps & kDebugRingBufferCapabilityMask) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
          //EXPECT_GT(ring_buffers.size(), 0u);
          const std::vector<WifiDebugRingBufferStatus> ring_buffers_vec =
              ring_buffers;
          for (const auto& ring_buffer : ring_buffers_vec) {
            EXPECT_GT(ring_buffer.ringName.size(), 0u);
            EXPECT_GT(ring_buffer.freeSizeInBytes, 0u);
          }
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
}

/*
 * StartLoggingToDebugRingBuffer
 * Invokes the |IWifiChip.startLoggingToDebugRingBuffer| method and ensures that
 * it succeeds.
 */
TEST_F(WifiChipHidlTest, StartLoggingToDebugRingBuffer) {
  uint32_t caps = configureChipAndGetCapabilities();
  std::string ring_name;
  wifi_chip_->getDebugRingBuffersStatus(
      [&](const WifiStatus& status,
          const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
        if (caps & kDebugRingBufferCapabilityMask) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
          EXPECT_GT(ring_buffers.size(), 0u);
          ring_name = ring_buffers[0].ringName.c_str();
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
  wifi_chip_->startLoggingToDebugRingBuffer(
      ring_name,
      kDebugRingBufferVerboseLvl,
      kDebugRingBufferMaxInterval,
      kDebugRingBufferMaxDataSize,
      [&](const WifiStatus& status) {
        if (caps & kDebugRingBufferCapabilityMask) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
}

/*
 * ForceDumpToDebugRingBuffer
 * Invokes the |IWifiChip.forceDumpToDebugRingBuffer| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, ForceDumpToDebugRingBuffer) {
  uint32_t caps = configureChipAndGetCapabilities();
  std::string ring_name;
  wifi_chip_->getDebugRingBuffersStatus(
      [&](const WifiStatus& status,
          const hidl_vec<WifiDebugRingBufferStatus>& ring_buffers) {
        if (caps & kDebugRingBufferCapabilityMask) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
          EXPECT_GT(ring_buffers.size(), 0u);
          ring_name = ring_buffers[0].ringName.c_str();
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
  wifi_chip_->forceDumpToDebugRingBuffer(
      ring_name, [&](const WifiStatus& status) {
        if (caps & kDebugRingBufferCapabilityMask) {
          EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        } else {
          EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
        }
      });
}

/*
 * GetDebugHostWakeReasonStats
 * Invokes the |IWifiChip.getDebugHostWakeReasonStats| method and ensures that
 * it succeeds.
 */
TEST_F(WifiChipHidlTest, GetDebugHostWakeReasonStats) {
  uint32_t caps = configureChipAndGetCapabilities();
  wifi_chip_->getDebugHostWakeReasonStats([&](
      const WifiStatus& status,
      const WifiDebugHostWakeReasonStats& /*  stats */) {
    if (caps & IWifiChip::ChipCapabilityMask::DEBUG_HOST_WAKE_REASON_STATS) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    } else {
      EXPECT_EQ(status.code, WifiStatusCode::ERROR_NOT_SUPPORTED);
    }
  });
}

/*
 * CreateApIface
 * Invokes the |IWifiChip.createApIface| method and ensures that it
 * succeeds.
 * Configures the chip in AP mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateApIface) {
  configureChip(IfaceType::AP);
  sp<IWifiApIface> iface;
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::ERROR_NOT_AVAILABLE);
}

/*
 * GetApIfaceNames
 * Invokes the |IWifiChip.getApIfaceNames| method and ensures that it
 * succeeds.
 * Configures the chip in AP mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetApIfaceNames) {
  configureChip(IfaceType::AP);
  wifi_chip_->getApIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
  sp<IWifiApIface> iface;
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getApIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 1u);
        EXPECT_EQ(names[0], iface_name);
      });
  EXPECT_EQ(removeApIface(iface_name), WifiStatusCode::SUCCESS);
  wifi_chip_->getApIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
}

/*
 * GetApIface
 * Invokes the |IWifiChip.getApIface| method and ensures that it
 * succeeds.
 * Configures the chip in AP mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetApIface) {
  configureChip(IfaceType::AP);
  sp<IWifiApIface> iface;
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getApIface(
      iface_name, [&](const WifiStatus& status, const sp<IWifiApIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_NE(iface.get(), nullptr);
      });
  std::string invalid_name = iface_name + "0";
  wifi_chip_->getApIface(
      invalid_name,
      [&](const WifiStatus& status, const sp<IWifiApIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::ERROR_INVALID_ARGS);
        EXPECT_EQ(iface.get(), nullptr);
      });
}

/*
 * RemoveApIface
 * Invokes the |IWifiChip.removeApIface| method and ensures that it
 * succeeds.
 * Configures the chip in AP mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveApIface) {
  configureChip(IfaceType::AP);
  sp<IWifiApIface> iface;
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  std::string invalid_name = iface_name + "0";
  EXPECT_EQ(removeApIface(invalid_name), WifiStatusCode::ERROR_INVALID_ARGS);
  EXPECT_EQ(removeApIface(iface_name), WifiStatusCode::SUCCESS);
  // No such iface exists now. So, this should return failure.
  EXPECT_EQ(removeApIface(iface_name), WifiStatusCode::ERROR_INVALID_ARGS);
}

#ifdef BOARD_HAS_NAN
/*
 * CreateNanIface
 * Invokes the |IWifiChip.createNanIface| method and ensures that it
 * succeeds.
 * Configures the chip in NAN mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateNanIface) {
  configureChip(IfaceType::NAN);
  sp<IWifiNanIface> iface;
  EXPECT_EQ(createNanIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  EXPECT_EQ(createNanIface(&iface), WifiStatusCode::ERROR_NOT_AVAILABLE);
}

/*
 * GetNanIfaceNames
 * Invokes the |IWifiChip.getNanIfaceNames| method and ensures that it
 * succeeds.
 * Configures the chip in NAN mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetNanIfaceNames) {
  configureChip(IfaceType::NAN);
  wifi_chip_->getNanIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
  sp<IWifiNanIface> iface;
  EXPECT_EQ(createNanIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getNanIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 1u);
        EXPECT_EQ(names[0], iface_name);
      });
  EXPECT_EQ(removeNanIface(iface_name), WifiStatusCode::SUCCESS);
  wifi_chip_->getNanIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
}

/*
 * GetNanIface
 * Invokes the |IWifiChip.getNanIface| method and ensures that it
 * succeeds.
 * Configures the chip in NAN mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetNanIface) {
  configureChip(IfaceType::NAN);
  sp<IWifiNanIface> iface;
  EXPECT_EQ(createNanIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getNanIface(
      iface_name,
      [&](const WifiStatus& status, const sp<IWifiNanIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_NE(iface.get(), nullptr);
      });
  std::string invalid_name = iface_name + "0";
  wifi_chip_->getNanIface(
      invalid_name,
      [&](const WifiStatus& status, const sp<IWifiNanIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::ERROR_INVALID_ARGS);
        EXPECT_EQ(iface.get(), nullptr);
      });
}

/*
 * RemoveNanIface
 * Invokes the |IWifiChip.removeNanIface| method and ensures that it
 * succeeds.
 * Configures the chip in NAN mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveNanIface) {
  configureChip(IfaceType::NAN);
  sp<IWifiNanIface> iface;
  EXPECT_EQ(createNanIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  std::string invalid_name = iface_name + "0";
  EXPECT_EQ(removeNanIface(invalid_name), WifiStatusCode::ERROR_INVALID_ARGS);
  EXPECT_EQ(removeNanIface(iface_name), WifiStatusCode::SUCCESS);
  // No such iface exists now. So, this should return failure.
  EXPECT_EQ(removeNanIface(iface_name), WifiStatusCode::ERROR_INVALID_ARGS);
}
#endif  // BOARD_HAS_NAN

/*
 * CreateP2pIface
 * Invokes the |IWifiChip.createP2pIface| method and ensures that it
 * succeeds.
 * Configures the chip in P2P mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateP2pIface) {
  configureChip(IfaceType::P2P);
  sp<IWifiP2pIface> iface;
  EXPECT_EQ(createP2pIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  EXPECT_EQ(createP2pIface(&iface), WifiStatusCode::ERROR_NOT_AVAILABLE);
}

/*
 * GetP2pIfaceNames
 * Invokes the |IWifiChip.getP2pIfaceNames| method and ensures that it
 * succeeds.
 * Configures the chip in P2P mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetP2pIfaceNames) {
  configureChip(IfaceType::P2P);
  wifi_chip_->getP2pIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
  sp<IWifiP2pIface> iface;
  EXPECT_EQ(createP2pIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getP2pIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 1u);
        EXPECT_EQ(names[0], iface_name);
      });
  EXPECT_EQ(removeP2pIface(iface_name), WifiStatusCode::SUCCESS);
  wifi_chip_->getP2pIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
}

/*
 * GetP2pIface
 * Invokes the |IWifiChip.getP2pIface| method and ensures that it
 * succeeds.
 * Configures the chip in P2P mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetP2pIface) {
  configureChip(IfaceType::P2P);
  sp<IWifiP2pIface> iface;
  EXPECT_EQ(createP2pIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getP2pIface(
      iface_name,
      [&](const WifiStatus& status, const sp<IWifiP2pIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_NE(iface.get(), nullptr);
      });
  std::string invalid_name = iface_name + "0";
  wifi_chip_->getP2pIface(
      invalid_name,
      [&](const WifiStatus& status, const sp<IWifiP2pIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::ERROR_INVALID_ARGS);
        EXPECT_EQ(iface.get(), nullptr);
      });
}

/*
 * RemoveP2pIface
 * Invokes the |IWifiChip.removeP2pIface| method and ensures that it
 * succeeds.
 * Configures the chip in P2P mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveP2pIface) {
  configureChip(IfaceType::P2P);
  sp<IWifiP2pIface> iface;
  EXPECT_EQ(createP2pIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  std::string invalid_name = iface_name + "0";
  EXPECT_EQ(removeP2pIface(invalid_name), WifiStatusCode::ERROR_INVALID_ARGS);
  EXPECT_EQ(removeP2pIface(iface_name), WifiStatusCode::SUCCESS);
  // No such iface exists now. So, this should return failure.
  EXPECT_EQ(removeP2pIface(iface_name), WifiStatusCode::ERROR_INVALID_ARGS);
}

/*
 * CreateStaIface
 * Invokes the |IWifiChip.createStaIface| method and ensures that it
 * succeeds.
 * Configures the chip in STA mode and ensures that only 1 iface creation
 * succeeds. The 2nd iface creation should be rejected.
 */
TEST_F(WifiChipHidlTest, CreateStaIface) {
  configureChip(IfaceType::STA);
  sp<IWifiStaIface> iface;
  EXPECT_EQ(createStaIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  EXPECT_EQ(createStaIface(&iface), WifiStatusCode::ERROR_NOT_AVAILABLE);
}

/*
 * GetStaIfaceNames
 * Invokes the |IWifiChip.getStaIfaceNames| method and ensures that it
 * succeeds.
 * Configures the chip in STA mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_F(WifiChipHidlTest, GetStaIfaceNames) {
  configureChip(IfaceType::STA);
  wifi_chip_->getStaIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
  sp<IWifiStaIface> iface;
  EXPECT_EQ(createStaIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getStaIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 1u);
        EXPECT_EQ(names[0], iface_name);
      });
  EXPECT_EQ(removeStaIface(iface_name), WifiStatusCode::SUCCESS);
  wifi_chip_->getStaIfaceNames(
      [&](const WifiStatus& status, const hidl_vec<hidl_string>& names) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_EQ(names.size(), 0u);
      });
}

/*
 * GetStaIface
 * Invokes the |IWifiChip.getStaIface| method and ensures that it
 * succeeds.
 * Configures the chip in STA mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_F(WifiChipHidlTest, GetStaIface) {
  configureChip(IfaceType::STA);
  sp<IWifiStaIface> iface;
  EXPECT_EQ(createStaIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  wifi_chip_->getStaIface(
      iface_name,
      [&](const WifiStatus& status, const sp<IWifiStaIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_NE(iface.get(), nullptr);
      });
  std::string invalid_name = iface_name + "0";
  wifi_chip_->getStaIface(
      invalid_name,
      [&](const WifiStatus& status, const sp<IWifiStaIface>& iface) {
        EXPECT_EQ(status.code, WifiStatusCode::ERROR_INVALID_ARGS);
        EXPECT_EQ(iface.get(), nullptr);
      });
}

/*
 * RemoveStaIface
 * Invokes the |IWifiChip.removeStaIface| method and ensures that it
 * succeeds.
 * Configures the chip in STA mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_F(WifiChipHidlTest, RemoveStaIface) {
  configureChip(IfaceType::STA);
  sp<IWifiStaIface> iface;
  EXPECT_EQ(createStaIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  std::string iface_name = getIfaceName(iface);
  std::string invalid_name = iface_name + "0";
  EXPECT_EQ(removeStaIface(invalid_name), WifiStatusCode::ERROR_INVALID_ARGS);
  EXPECT_EQ(removeStaIface(iface_name), WifiStatusCode::SUCCESS);
  // No such iface exists now. So, this should return failure.
  EXPECT_EQ(removeStaIface(iface_name), WifiStatusCode::ERROR_INVALID_ARGS);
}

/*
 * CreateRttController
 * Invokes the |IWifiChip.createRttController| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, CreateRttController) {
  configureChip(IfaceType::AP);
  sp<IWifiApIface> iface;
  EXPECT_EQ(createApIface(&iface), WifiStatusCode::SUCCESS);
  EXPECT_NE(iface.get(), nullptr);
  sp<IWifiRttController> wifi_rtt_controller;
  wifi_chip_->createRttController(
      iface,
      [&](const WifiStatus& status,
          const sp<IWifiRttController>& rtt_controller) {
        EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
        EXPECT_NE(rtt_controller.get(), nullptr);
      });
}
