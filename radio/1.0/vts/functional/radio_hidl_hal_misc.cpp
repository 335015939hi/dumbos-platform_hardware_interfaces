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

#include <radio_hidl_hal_utils.h>

/*
 * Test IRadio.getSignalStrength() for the response returned.
 */
TEST_F(RadioHidlTest, getSignalStrength) {
  int serial = 1;

  radio->getSignalStrength(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getVoiceRegistrationState() for the response returned.
 */
TEST_F(RadioHidlTest, getVoiceRegistrationState) {
  int serial = 1;

  radio->getVoiceRegistrationState(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getOperator() for the response returned.
 */
TEST_F(RadioHidlTest, getOperator) {
  int serial = 1;

  radio->getOperator(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setRadioPower() for the response returned.
 */
TEST_F(RadioHidlTest, setRadioPower) {
  int serial = 1;

  radio->setRadioPower(++serial, 0);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getNetworkSelectionMode() for the response returned.
 */
TEST_F(RadioHidlTest, getNetworkSelectionMode) {
  int serial = 1;

  radio->getNetworkSelectionMode(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setNetworkSelectionModeAutomatic() for the response returned.
 */
TEST_F(RadioHidlTest, setNetworkSelectionModeAutomatic) {
  int serial = 1;

  radio->setNetworkSelectionModeAutomatic(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::ILLEGAL_SIM_OR_ME);
  }
}

/*
 * Test IRadio.setNetworkSelectionModeManual() for the response returned.
 */
TEST_F(RadioHidlTest, setNetworkSelectionModeManual) {
  int serial = 1;

  radio->setNetworkSelectionModeManual(++serial, "123456");
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::ILLEGAL_SIM_OR_ME);
  }
}

/*
 * Test IRadio.getAvailableNetworks() for the response returned.
 */
TEST_F(RadioHidlTest, getAvailableNetworks) {
  int serial = 1;

  radio->getAvailableNetworks(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getBasebandVersion() for the response returned.
 */
TEST_F(RadioHidlTest, getBasebandVersion) {
  int serial = 1;

  radio->getBasebandVersion(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setBandMode() for the response returned.
 */
TEST_F(RadioHidlTest, setBandMode) {
  int serial = 1;

  radio->setBandMode(++serial, RadioBandMode::BAND_MODE_USA);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getAvailableBandModes() for the response returned.
 */
TEST_F(RadioHidlTest, getAvailableBandModes) {
  int serial = 1;

  radio->getAvailableBandModes(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setPreferredNetworkType() for the response returned.
 */
TEST_F(RadioHidlTest, setPreferredNetworkType) {
  int serial = 1;

  radio->setPreferredNetworkType(++serial, PreferredNetworkType::GSM_ONLY);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getPreferredNetworkType() for the response returned.
 */
TEST_F(RadioHidlTest, getPreferredNetworkType) {
  int serial = 1;

  radio->getPreferredNetworkType(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getNeighboringCids() for the response returned.
 */
TEST_F(RadioHidlTest, getNeighboringCids) {
  int serial = 1;

  radio->getNeighboringCids(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setLocationUpdates() for the response returned.
 */
TEST_F(RadioHidlTest, setLocationUpdates) {
  int serial = 1;

  radio->setLocationUpdates(++serial, true);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setCdmaRoamingPreference() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaRoamingPreference) {
  int serial = 1;

  radio->setCdmaRoamingPreference(++serial, CdmaRoamingType::HOME_NETWORK);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getCdmaRoamingPreference() for the response returned.
 */
TEST_F(RadioHidlTest, getCdmaRoamingPreference) {
  int serial = 1;

  radio->getCdmaRoamingPreference(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getTTYMode() for the response returned.
 */
TEST_F(RadioHidlTest, getTTYMode) {
  int serial = 1;

  radio->getTTYMode(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setTTYMode() for the response returned.
 */
TEST_F(RadioHidlTest, setTTYMode) {
  int serial = 1;

  radio->setTTYMode(++serial, TtyMode::OFF);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setPreferredVoicePrivacy() for the response returned.
 */
TEST_F(RadioHidlTest, setPreferredVoicePrivacy) {
  int serial = 1;

  radio->setPreferredVoicePrivacy(++serial, true);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getPreferredVoicePrivacy() for the response returned.
 */
TEST_F(RadioHidlTest, getPreferredVoicePrivacy) {
  int serial = 1;

  radio->getPreferredVoicePrivacy(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getCDMASubscription() for the response returned.
 */
TEST_F(RadioHidlTest, getCDMASubscription) {
  int serial = 1;

  radio->getCDMASubscription(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getDeviceIdentity() for the response returned.
 */
TEST_F(RadioHidlTest, getDeviceIdentity) {
  int serial = 1;

  radio->getDeviceIdentity(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.exitEmergencyCallbackMode() for the response returned.
 */
TEST_F(RadioHidlTest, exitEmergencyCallbackMode) {
  int serial = 1;

  radio->exitEmergencyCallbackMode(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getCdmaSubscriptionSource() for the response returned.
 */
TEST_F(RadioHidlTest, getCdmaSubscriptionSource) {
  int serial = 1;

  radio->getCdmaSubscriptionSource(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setCdmaSubscriptionSource() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaSubscriptionSource) {
  int serial = 1;

  radio->setCdmaSubscriptionSource(++serial, CdmaSubscriptionSource::RUIM_SIM);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::SIM_ABSENT ||
                rspInfo->error == RadioError::SUBSCRIPTION_NOT_AVAILABLE);
  }
}

/*
 * Test IRadio.getVoiceRadioTechnology() for the response returned.
 */
TEST_F(RadioHidlTest, getVoiceRadioTechnology) {
  int serial = 1;

  radio->getVoiceRadioTechnology(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getCellInfoList() for the response returned.
 */
TEST_F(RadioHidlTest, getCellInfoList) {
  int serial = 1;

  radio->getCellInfoList(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setCellInfoListRate() for the response returned.
 */
TEST_F(RadioHidlTest, setCellInfoListRate) {
  int serial = 1;

  // TODO(sanketpadawe): RIL crashes with value of rate = 10
  radio->setCellInfoListRate(++serial, 10);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.nvReadItem() for the response returned.
 */
TEST_F(RadioHidlTest, nvReadItem) {
  int serial = 1;

  radio->nvReadItem(++serial, NvItem::LTE_BAND_ENABLE_25);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.nvWriteItem() for the response returned.
 */
TEST_F(RadioHidlTest, nvWriteItem) {
  int serial = 1;
  NvWriteItem item;
  memset(&item, 0, sizeof(item));
  item.value = hidl_string();

  radio->nvWriteItem(++serial, item);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.nvWriteCdmaPrl() for the response returned.
 */
TEST_F(RadioHidlTest, nvWriteCdmaPrl) {
  int serial = 1;
  std::vector<uint8_t> prl = {1, 2, 3, 4, 5};

  radio->nvWriteCdmaPrl(++serial, hidl_vec<uint8_t>(prl));
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.nvResetConfig() for the response returned.
 */
TEST_F(RadioHidlTest, nvResetConfig) {
  int serial = 1;

  radio->nvResetConfig(++serial, ResetNvType::RELOAD);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setUiccSubscription() for the response returned.
 */
TEST_F(RadioHidlTest, setUiccSubscription) {
  int serial = 1;
  SelectUiccSub item;
  memset(&item, 0, sizeof(item));

  radio->setUiccSubscription(++serial, item);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getHardwareConfig() for the response returned.
 */
TEST_F(RadioHidlTest, getHardwareConfig) {
  int serial = 1;

  radio->getHardwareConfig(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.requestShutdown() for the response returned.
 */
TEST_F(RadioHidlTest, requestShutdown) {
  int serial = 1;

  radio->requestShutdown(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getRadioCapability() for the response returned.
 */
TEST_F(RadioHidlTest, getRadioCapability) {
  int serial = 1;

  radio->getRadioCapability(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setRadioCapability() for the response returned.
 */
TEST_F(RadioHidlTest, setRadioCapability) {
  int serial = 1;
  RadioCapability rc;
  memset(&rc, 0, sizeof(rc));
  rc.logicalModemUuid = hidl_string();

  radio->setRadioCapability(++serial, rc);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.startLceService() for the response returned.
 */
TEST_F(RadioHidlTest, startLceService) {
  int serial = 1;

  radio->startLceService(++serial, 5, true);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE ||
                rspInfo->error == RadioError::LCE_NOT_SUPPORTED);
  }
}

/*
 * Test IRadio.stopLceService() for the response returned.
 */
TEST_F(RadioHidlTest, stopLceService) {
  int serial = 1;

  radio->stopLceService(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE ||
                rspInfo->error == RadioError::LCE_NOT_SUPPORTED);
  }
}

/*
 * Test IRadio.pullLceData() for the response returned.
 */
TEST_F(RadioHidlTest, pullLceData) {
  int serial = 1;

  radio->pullLceData(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE ||
                rspInfo->error == RadioError::LCE_NOT_SUPPORTED);
  }
}

/*
 * Test IRadio.getModemActivityInfo() for the response returned.
 */
TEST_F(RadioHidlTest, getModemActivityInfo) {
  int serial = 1;

  radio->getModemActivityInfo(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setAllowedCarriers() for the response returned.
 */
TEST_F(RadioHidlTest, setAllowedCarriers) {
  int serial = 1;
  CarrierRestrictions carriers;
  memset(&carriers, 0, sizeof(carriers));
  carriers.allowedCarriers.resize(1);
  carriers.excludedCarriers.resize(0);
  carriers.allowedCarriers[0].mcc = hidl_string();
  carriers.allowedCarriers[0].mnc = hidl_string();
  carriers.allowedCarriers[0].matchType = CarrierMatchType::ALL;
  carriers.allowedCarriers[0].matchData = hidl_string();

  radio->setAllowedCarriers(++serial, false, carriers);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.getAllowedCarriers() for the response returned.
 */
TEST_F(RadioHidlTest, getAllowedCarriers) {
  int serial = 1;

  radio->getAllowedCarriers(++serial);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.sendDeviceState() for the response returned.
 */
TEST_F(RadioHidlTest, sendDeviceState) {
  int serial = 1;

  radio->sendDeviceState(++serial, DeviceStateType::POWER_SAVE_MODE, true);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setIndicationFilter() for the response returned.
 */
TEST_F(RadioHidlTest, setIndicationFilter) {
  int serial = 1;

  radio->setIndicationFilter(++serial, 1);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::NONE);
  }
}

/*
 * Test IRadio.setSimCardPower() for the response returned.
 */
TEST_F(RadioHidlTest, setSimCardPower) {
  int serial = 1;

  radio->setSimCardPower(++serial, true);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(serial, rspInfo->serial);

  if (cardStatus.cardState == CardState::ABSENT) {
    ASSERT_TRUE(rspInfo->error == RadioError::SIM_ABSENT);
  }
}
