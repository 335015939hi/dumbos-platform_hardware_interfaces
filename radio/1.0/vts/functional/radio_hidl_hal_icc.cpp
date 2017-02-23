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

/*
 * Test IRadio.getIccCardStatus() for the response returned.
 */
TEST_F(RadioHidlTest, getIccCardStatus) {
    radio->getIccCardStatus(1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    EXPECT_LE(radioRsp->cardStatus.applications.size(), (unsigned int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.gsmUmtsSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.cdmaSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.imsSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
}

/*
 * Test IRadio.supplyIccPinForApp() for the response returned
 */
TEST_F(RadioHidlTest, supplyIccPinForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->supplyIccPinForApp(serial++, hidl_string("test1"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.supplyIccPukForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPukForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->supplyIccPukForApp(serial++, hidl_string("test1"), hidl_string("test2"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.supplyIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPin2ForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->supplyIccPin2ForApp(serial++, hidl_string("test1"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.supplyIccPuk2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPuk2ForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->supplyIccPuk2ForApp(serial++, hidl_string("test1"), hidl_string("test2"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.changeIccPinForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPinForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->changeIccPinForApp(serial++, hidl_string("test1"), hidl_string("test2"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.changeIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPin2ForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->changeIccPin2ForApp(serial++, hidl_string("test1"), hidl_string("test2"),
                    radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
        }
    }
}

/*
 * Test IRadio.getImsiForApp() for the response returned.
 */
TEST_F(RadioHidlTest, getImsiForApp) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        if (radioRsp->cardStatus.applications[i].appType == AppType::SIM
                || radioRsp->cardStatus.applications[i].appType == AppType::USIM
                || radioRsp->cardStatus.applications[i].appType == AppType::RUIM
                || radioRsp->cardStatus.applications[i].appType == AppType::CSIM) {
            radio->getImsiForApp(serial++, radioRsp->cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
            EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
            EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

            // IMSI (MCC+MNC+MSIN) is at least 6 digits, but not more than 15
            if (radioRsp->rspInfo.error == RadioError::NONE) {
                EXPECT_NE(radioRsp->imsi, hidl_string());
                EXPECT_GE((int) (radioRsp->imsi).size(), 6);
                EXPECT_LE((int) (radioRsp->imsi).size(), 15);
            }
        }
    }
}

/*
 * Test IRadio.iccIOForApp() for the response returned.
 */
TEST_F(RadioHidlTest, iccIOForApp) {
    int serial = 1;
    IccIo iccIo;
    iccIo.command = 0xc0;
    iccIo.fileId = 0x6f11;
    iccIo.path = hidl_string("3F007FFF");
    iccIo.p1 = 0;
    iccIo.p2 = 0;
    iccIo.p3 = 0;
    iccIo.data = hidl_string();
    iccIo.pin2 = hidl_string();
    iccIo.aid = hidl_string();

    radio->iccIOForApp(serial, iccIo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.iccTransmitApduBasicChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccTransmitApduBasicChannel) {
    int serial = 1;
    SimApdu msg;
    msg.sessionId = 0;
    msg.cla = 0;
    msg.instruction = 0;
    msg.p1 = 0;
    msg.p2 = 0;
    msg.p3 = 0;
    msg.data = hidl_string();

    radio->iccTransmitApduBasicChannel(serial, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.iccOpenLogicalChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccOpenLogicalChannel) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        radio->iccOpenLogicalChannel(serial++, radioRsp->cardStatus.applications[i].aidPtr);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
        // TODO: Add tests related to error code returned
    }
}

/*
 * Test IRadio.iccCloseLogicalChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccCloseLogicalChannel) {
    int serial = 1;
    radio->iccCloseLogicalChannel(serial, 0);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_ARGUMENTS);
}

/*
 * Test IRadio.iccTransmitApduLogicalChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccTransmitApduLogicalChannel) {
    SimApdu msg;
    msg.sessionId = 0;
    msg.cla = 0;
    msg.instruction = 0;
    msg.p1 = 0;
    msg.p2 = 0;
    msg.p3 = 0;
    msg.data = hidl_string();

    radio->iccTransmitApduLogicalChannel(1, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(1, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.requestIccSimAuthentication() for the response returned.
 */
TEST_F(RadioHidlTest, requestIccSimAuthentication) {
    int serial = 1;
    radio->getIccCardStatus(serial++);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    for (int i = 0; i < (int) radioRsp->cardStatus.applications.size(); i++) {
        radio->requestIccSimAuthentication(serial++, 0, hidl_string("test"),
                radioRsp->cardStatus.applications[i].aidPtr);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(serial - 1, radioRsp->rspInfo.serial);
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
        EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_ARGUMENTS);
    }
}