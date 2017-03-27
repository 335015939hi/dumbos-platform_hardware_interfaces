/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Nanache License, Version 2.0 (the "License");
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

#include <android/hardware/wifi/1.0/IWifiNanIface.h>
#include <android/hardware/wifi/1.0/IWifiNanIfaceEventCallback.h>

#include <VtsHalHidlTargetTestBase.h>
#include <set>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"
#include "wifi_nan_test_utils.h"

using namespace ::android::hardware::wifi::V1_0;

using ::android::sp;
using ::nan::CallbackType;
using ::nan::WifiNanIfaceEventCallback;

static uint16_t commandId = 0;

/**
 * Fixture to use for all NAN Iface HIDL interface tests.
 */
class WifiNanIfaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
      iwifiNanIface = getWifiNanIface();
      callback = new WifiNanIfaceEventCallback();
      ASSERT_NE(nullptr, iwifiNanIface.get());
      ASSERT_EQ(WifiStatusCode::SUCCESS,
                HIDL_INVOKE(iwifiNanIface,
                            registerEventCallback,
                            callback).code);
    }

  virtual void TearDown() override {
      stopWifi();
    }

    protected:
      android::sp<IWifiNanIface> iwifiNanIface;
      WifiNanIfaceEventCallback* callback;
};

/*
 * Create:
 * Ensures that an instance of the IWifiNanIface proxy object is
 * successfully created.
 */
TEST(WifiNanIfaceHidlTestNoFixture, Create) {
  ASSERT_NE(nullptr, getWifiNanIface().get());
  stopWifi();
}

/*
 * Fail: use past destruction
 * Ensure that API calls fail with ERROR_WIFI_IFACE_INVALID when using an interface once wifi
 * is disabled.
 */
TEST(WifiNanIfaceHidlTestNoFixture, FailOnIfaceInvalid) {
  android::sp<IWifiNanIface> iwifiNanIface = getWifiNanIface();
  ASSERT_NE(nullptr, iwifiNanIface.get());
  stopWifi();
  sleep(5); // make sure that all chips/interfaces are invalidated
  ASSERT_EQ(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
          HIDL_INVOKE(iwifiNanIface, getCapabilitiesRequest, 0).code);
}

/*
 * getCapabilitiesRequest: validate that returns capabilities.
 */
TEST_F(WifiNanIfaceHidlTest, getCapabilitiesRequest) {
  uint16_t inputCmdId = commandId++;
  ASSERT_EQ(WifiStatusCode::SUCCESS,
        HIDL_INVOKE(iwifiNanIface, getCapabilitiesRequest, inputCmdId).code);
  // wait for a callback
  auto cbd = callback->wait(CallbackType::NOTIFY_CAPABILITIES_RESPONSE);
  ASSERT_NE(cbd->callbackType, CallbackType::TIMEOUT);
  ASSERT_EQ(cbd->callbackType, CallbackType::NOTIFY_CAPABILITIES_RESPONSE);
  ASSERT_EQ(cbd->id, inputCmdId);

  // check for reasonable capability values
  EXPECT_GT(cbd->capabilities.maxConcurrentClusters, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxPublishes, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxSubscribes, (unsigned int) 0);
  EXPECT_EQ(cbd->capabilities.maxServiceNameLen, (unsigned int) 255);
  EXPECT_EQ(cbd->capabilities.maxMatchFilterLen, (unsigned int) 255);
  EXPECT_GT(cbd->capabilities.maxTotalMatchFilterLen, (unsigned int) 255);
  EXPECT_EQ(cbd->capabilities.maxServiceSpecificInfoLen, (unsigned int) 255);
  EXPECT_GE(cbd->capabilities.maxExtendedServiceSpecificInfoLen,
    (unsigned int) 255);
  EXPECT_GT(cbd->capabilities.maxNdiInterfaces, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxNdpSessions, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxAppInfoLen, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxQueuedTransmitFollowupMsgs, (unsigned int) 0);
  EXPECT_GT(cbd->capabilities.maxSubscribeInterfaceAddresses, (unsigned int) 0);
  EXPECT_NE(cbd->capabilities.supportedCipherSuites, (unsigned int) 0);
}

/*
 * EnableRequest fixture and parameterized tests
 */

/*
 * Contains the EnableRequest parameters which we'll be varying in the framework - i.e. high
 * priority for testing.
 */
class EnableRequestParameters {
  public:
    EnableRequestParameters(uint8_t masterPref, bool disableDiscoveryAddressChangeIndication,
        bool disableStartedClusterIndication, bool disableJoinedClusterIndication,
        uint16_t clusterIdTopRangeVal, uint16_t clusterIdBottomRangeVal) :
        masterPref(masterPref),
        disableDiscoveryAddressChangeIndication(disableDiscoveryAddressChangeIndication),
        disableStartedClusterIndication(disableStartedClusterIndication),
        disableJoinedClusterIndication(disableJoinedClusterIndication),
        clusterIdTopRangeVal(clusterIdTopRangeVal),
        clusterIdBottomRangeVal(clusterIdBottomRangeVal) {}

    uint8_t masterPref;

    bool disableDiscoveryAddressChangeIndication;
    bool disableStartedClusterIndication;
    bool disableJoinedClusterIndication;

    uint16_t clusterIdTopRangeVal;
    uint16_t clusterIdBottomRangeVal;
};

/* EnableRequest parameterized fixture */
class EnableRequestFixture: public WifiNanIfaceHidlTest,
                            public ::testing::WithParamInterface<EnableRequestParameters> {
  public:
    /*
     * Returns a NanEnableRequest initialized to the baseline/default values
     * used by the framework.
     */
    std::shared_ptr<NanEnableRequest> getInitializedNanEnableRequest() {
      NanEnableRequest* msg = new NanEnableRequest;

      // fill-in NanEnableRequest with typical values used by the framework
      msg->operateInBand[(size_t) NanBandIndex::NAN_BAND_24GHZ] = true;
      msg->operateInBand[(size_t) NanBandIndex::NAN_BAND_5GHZ] = true;
      msg->hopCountMax = 2;
      msg->configParams.masterPref = 10;
      msg->configParams.disableDiscoveryAddressChangeIndication = false;
      msg->configParams.disableStartedClusterIndication = false;
      msg->configParams.disableJoinedClusterIndication = false;
      msg->configParams.includePublishServiceIdsInBeacon = true;
      msg->configParams.numberOfPublishServiceIdsInBeacon = 0;
      msg->configParams.includeSubscribeServiceIdsInBeacon = true;
      msg->configParams.numberOfSubscribeServiceIdsInBeacon = 0;
      msg->configParams.rssiWindowSize = 8;
      msg->configParams.macAddressRandomizationIntervalSec = 1800;

      NanBandSpecificConfig& config24 = msg->configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_24GHZ];
      config24.rssiClose = 60;
      config24.rssiMiddle = 70;
      config24.rssiCloseProximity = 60;
      config24.dwellTimeMs = 200;
      config24.scanPeriodSec = 20;
      config24.validDiscoveryWindowIntervalVal = false;

      NanBandSpecificConfig& config5 = msg->configParams.bandSpecificConfig[
            (size_t) NanBandIndex::NAN_BAND_5GHZ];
      config5.rssiClose = 60;
      config5.rssiMiddle = 75;
      config5.rssiCloseProximity = 60;
      config5.dwellTimeMs = 200;
      config5.scanPeriodSec = 20;
      config5.validDiscoveryWindowIntervalVal = false;

      msg->debugConfigs.validClusterIdVals = true;
      msg->debugConfigs.clusterIdTopRangeVal = 0xFFFF;
      msg->debugConfigs.clusterIdBottomRangeVal = 0x0000;
      msg->debugConfigs.validIntfAddrVal = false;
      msg->debugConfigs.validOuiVal = false;
      msg->debugConfigs.ouiVal = 0;
      msg->debugConfigs.validRandomFactorForceVal = false;
      msg->debugConfigs.randomFactorForceVal = 0;
      msg->debugConfigs.validHopCountForceVal = false;
      msg->debugConfigs.hopCountForceVal = 0;
      msg->debugConfigs.validDiscoveryChannelVal = false;
      msg->debugConfigs.discoveryChannelMhzVal[(size_t) NanBandIndex::NAN_BAND_24GHZ] = 0;
      msg->debugConfigs.discoveryChannelMhzVal[(size_t) NanBandIndex::NAN_BAND_5GHZ] = 0;
      msg->debugConfigs.validUseBeaconsInBandVal = false;
      msg->debugConfigs.useBeaconsInBandVal[(size_t) NanBandIndex::NAN_BAND_24GHZ] = true;
      msg->debugConfigs.useBeaconsInBandVal[(size_t) NanBandIndex::NAN_BAND_5GHZ] = true;
      msg->debugConfigs.validUseSdfInBandVal = false;
      msg->debugConfigs.useSdfInBandVal[(size_t) NanBandIndex::NAN_BAND_24GHZ] = true;
      msg->debugConfigs.useSdfInBandVal[(size_t) NanBandIndex::NAN_BAND_5GHZ] = true;

      return std::shared_ptr<NanEnableRequest>(msg);
    }
};

/*
 * Validate that enableRequest calls succeeds with various set of parameters.
 */
TEST_P(EnableRequestFixture, enableRequestSuccess) {
  uint16_t inputCmdId = commandId++;
  std::shared_ptr<NanEnableRequest> msg = getInitializedNanEnableRequest();

  const EnableRequestParameters& overrides = GetParam();
  msg->configParams.masterPref = overrides.masterPref;
  msg->configParams.disableDiscoveryAddressChangeIndication =
        overrides.disableDiscoveryAddressChangeIndication;
  msg->configParams.disableStartedClusterIndication = overrides.disableStartedClusterIndication;
  msg->configParams.disableJoinedClusterIndication = overrides.disableJoinedClusterIndication;
  msg->debugConfigs.clusterIdTopRangeVal = overrides.clusterIdTopRangeVal;
  msg->debugConfigs.clusterIdBottomRangeVal = overrides.clusterIdBottomRangeVal;

  ASSERT_EQ(WifiStatusCode::SUCCESS,
        HIDL_INVOKE(iwifiNanIface, enableRequest, inputCmdId, *msg).code);
  // wait for a callback
  auto cbd = callback->wait(CallbackType::NOTIFY_ENABLE_RESPONSE);
  ASSERT_NE(cbd->callbackType, CallbackType::TIMEOUT);
  ASSERT_EQ(cbd->callbackType, CallbackType::NOTIFY_ENABLE_RESPONSE);
  ASSERT_EQ(cbd->id, inputCmdId);
  ASSERT_EQ(cbd->status.status, NanStatusType::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(enableRequestSuccessTestCases, EnableRequestFixture,
    ::testing::Values(
    EnableRequestParameters(0, false, false, false, 0xFFFF, 0x0000),
    EnableRequestParameters(1, false, false, true, 0xFFFF, 0x1000),
    EnableRequestParameters(10, false, true, true, 0x1000, 0x0000),
    EnableRequestParameters(10, true, false, false, 0xFFFF, 0x0000),
    EnableRequestParameters(10, true, false, true, 0xFFFF, 0x0000),
    EnableRequestParameters(10, true, true, false, 0xFFFF, 0x0000),
    EnableRequestParameters(10, true, true, true, 0xFFFF, 0x0000)
));

TEST_F(EnableRequestFixture, addressChangeOnEnable) {
  std::set<std::string> addresses;
  for (int i = 0; i < 10; ++i) {
    uint16_t inputCmdId = commandId++;
    std::shared_ptr<NanEnableRequest> msg = getInitializedNanEnableRequest();

    ASSERT_EQ(WifiStatusCode::SUCCESS,
         HIDL_INVOKE(iwifiNanIface, enableRequest, inputCmdId, *msg).code);
    // wait for a notification
    auto cbd = callback->wait(CallbackType::NOTIFY_ENABLE_RESPONSE);
    ASSERT_NE(cbd->callbackType, CallbackType::TIMEOUT);
    ASSERT_EQ(cbd->callbackType, CallbackType::NOTIFY_ENABLE_RESPONSE);
    ASSERT_EQ(cbd->id, inputCmdId);
    ASSERT_EQ(cbd->status.status, NanStatusType::SUCCESS);

    // wait for event
    do {
      cbd = callback->wait(CallbackType::EVENT_CLUSTER_EVENT);
      ASSERT_NE(cbd->callbackType, CallbackType::TIMEOUT);
      ASSERT_EQ(cbd->callbackType, CallbackType::EVENT_CLUSTER_EVENT);
    } while (cbd->nanClusterEventInd.eventType
          != NanClusterEventType::DISCOVERY_MAC_ADDRESS_CHANGED);

    ASSERT_EQ(cbd->nanClusterEventInd.addr.size(), (unsigned long) 6);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        cbd->nanClusterEventInd.addr[0],
        cbd->nanClusterEventInd.addr[1],
        cbd->nanClusterEventInd.addr[2],
        cbd->nanClusterEventInd.addr[3],
        cbd->nanClusterEventInd.addr[4],
        cbd->nanClusterEventInd.addr[5]);

    addresses.insert(macStr);

    // disable
    inputCmdId = commandId++;
    ASSERT_EQ(WifiStatusCode::SUCCESS,
          HIDL_INVOKE(iwifiNanIface, disableRequest, inputCmdId).code);
    // wait for a notification
    cbd = callback->wait(CallbackType::NOTIFY_DISABLE_RESPONSE);
    ASSERT_NE(cbd->callbackType, CallbackType::TIMEOUT);
    ASSERT_EQ(cbd->callbackType, CallbackType::NOTIFY_DISABLE_RESPONSE);
    ASSERT_EQ(cbd->id, inputCmdId);
    ASSERT_EQ(cbd->status.status, NanStatusType::SUCCESS);
  }

  ASSERT_EQ(addresses.size(), (unsigned int) 10);
}