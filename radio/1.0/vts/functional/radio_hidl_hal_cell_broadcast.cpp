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

#include<radio_hidl_hal_utils.h>

using namespace ::android::hardware::radio::V1_0;

/*
 * Test IRadio.getGsmBroadcastConfig() for the response returned.
 */
TEST_F(RadioHidlTest, getGsmBroadcastConfig) {
    int serial = 0;

    radio->getGsmBroadcastConfig(++serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.setGsmBroadcastConfig() for the response returned.
 */
TEST_F(RadioHidlTest, setGsmBroadcastConfig) {
    int serial = 0;
    android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo> gsmBroadcastSmsConfigsInfoList;

    radio->setGsmBroadcastConfig(++serial, gsmBroadcastSmsConfigsInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS 
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.setGsmBroadcastActivation() for the response returned.
 */
TEST_F(RadioHidlTest, setGsmBroadcastActivation) {
    int serial = 0;
    bool activate = false;

    radio->setGsmBroadcastActivation(++serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.getCdmaBroadcastConfig() for the response returned.
 */
TEST_F(RadioHidlTest, getCdmaBroadcastConfig) {
    int serial = 0;

    radio->getCdmaBroadcastConfig(++serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.setCdmaBroadcastConfig() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaBroadcastConfig) {
    int serial = 0;
    android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo> cdmaBroadcastSmsConfigInfoList;

    radio->setCdmaBroadcastConfig(++serial, cdmaBroadcastSmsConfigInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS 
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.setCdmaBroadcastActivation() for the response returned.
 */
TEST_F(RadioHidlTest, setCdmaBroadcastActivation) {
    int serial = 0;
    bool activate = false;

    radio->setCdmaBroadcastActivation(++serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}
