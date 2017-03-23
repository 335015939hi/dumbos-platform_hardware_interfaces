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

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"
#include "wifi_nan_test_utils.h"

using namespace ::android::hardware::wifi::V1_0;

using ::android::sp;
using ::nan::CallbackType;
using ::nan::WifiNanIfaceEventCallback;

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
  uint16_t inputCmdId = 10;
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
