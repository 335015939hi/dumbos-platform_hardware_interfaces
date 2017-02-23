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

    for (int i = 0; i < radioRsp->cardStatus.applications.size(); i++) {
        ALOGW("sanket app %d aid = %s", i, (const char *) radioRsp->cardStatus.applications[i].aidPtr);
    }
}

/*
 * Test IRadio.supplyIccPinForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPinForApp) {
    /*radio->getIccCardStatus(11);
    EXPECT_EQ(std::cv_status::no_timeout, wait());

    ALOGW("sanket before");
    radio->supplyIccPinForApp(12, hidl_string("test1"), radioRsp->cardStatus.applications[1].aidPtr);
    ALOGW("sanket after");*/

    radio->supplyIccPinForApp(2, hidl_string("test1"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(2, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPukForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPukForApp) {
    radio->supplyIccPukForApp(3, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(3, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPin2ForApp) {
    radio->supplyIccPin2ForApp(4, hidl_string("test1"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(4, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPuk2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPuk2ForApp) {
    radio->supplyIccPuk2ForApp(5, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(5, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.changeIccPinForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPinForApp) {
    radio->changeIccPinForApp(6, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(6, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.changeIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPin2ForApp) {
    radio->changeIccPin2ForApp(7, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(7, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.getImsiForApp() for the response returned.
 */
TEST_F(RadioHidlTest, getImsiForApp) {
    radio->getImsiForApp(8, hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(8, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned

    // IMSI (MCC+MNC+MSIN) is at least 6 digits, but not more than 15
    if (radioRsp->rspInfo.error == RadioError::NONE) {
        EXPECT_NE(radioRsp->imsi, hidl_string());
        EXPECT_GE((radioRsp->imsi).size(), 6);
        EXPECT_LE((radioRsp->imsi).size(), 15);
    }
}

/*
 * Test IRadio.iccIOForApp() for the response returned.
 */
TEST_F(RadioHidlTest, iccIOForApp) {
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

    radio->iccIOForApp(9, iccIo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(9, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.iccTransmitApduBasicChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccTransmitApduBasicChannel) {
    SimApdu msg;
    msg.sessionId = 0;
    msg.cla = 0;
    msg.instruction = 0;
    msg.p1 = 0;
    msg.p2 = 0;
    msg.p3 = 0;
    msg.data = hidl_string();

    radio->iccTransmitApduBasicChannel(10, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(10, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.iccOpenLogicalChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccOpenLogicalChannel) {
    radio->iccOpenLogicalChannel(11, hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(11, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::MISSING_RESOURCE);
}

/*
 * Test IRadio.iccCloseLogicalChannel() for the response returned.
 */
TEST_F(RadioHidlTest, iccCloseLogicalChannel) {
    radio->iccCloseLogicalChannel(12, 0);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(12, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
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

    radio->iccTransmitApduLogicalChannel(13, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(13, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.requestIccSimAuthentication() for the response returned.
 */
TEST_F(RadioHidlTest, requestIccSimAuthentication) {
    radio->requestIccSimAuthentication(14, 0, hidl_string(), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(14, radioRsp->rspInfo.serial);

    // TODO : Add tests related to error code returned
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_ARGUMENTS);
}