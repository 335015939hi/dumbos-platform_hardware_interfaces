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
 * Test IRadio.sendSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendSms) {
    int serial = 0;
    android::hardware::radio::V1_0::GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->sendSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);
    EXPECT_EQ(radioRsp->sendSmsResult.ackPDU, "");
    EXPECT_EQ(radioRsp->sendSmsResult.errorCode, -1);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.sendSMSExpectMore() for the response returned.
 */
TEST_F(RadioHidlTest, sendSMSExpectMore) {
    int serial = 0;
    android::hardware::radio::V1_0::GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->sendSMSExpectMore(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);
    EXPECT_EQ(radioRsp->sendSmsResult.ackPDU, "");
    EXPECT_EQ(radioRsp->sendSmsResult.errorCode, -1);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.acknowledgeLastIncomingGsmSms() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeLastIncomingGsmSms) {
    int serial = 0;
    bool success = true;

    radio->acknowledgeLastIncomingGsmSms(++serial, success, 
	android::hardware::radio::V1_0::SmsAcknowledgeFailCause::MEMORY_CAPACITY_EXCEEDED);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::MODEM_ERR);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.writeSmsToSim() for the response returned.
 */
TEST_F(RadioHidlTest, writeSmsToSim) {
    int serial = 0;
    android::hardware::radio::V1_0::SmsWriteArgs smsWriteArgs;
    smsWriteArgs.status = android::hardware::radio::V1_0::SmsWriteArgsStatus::REC_UNREAD;
    smsWriteArgs.smsc = "";
    smsWriteArgs.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio->writeSmsToSim(++serial, smsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::SYSTEM_ERR);

    // TODO : radioRsp->writeSmsToSimIndex needs to be investigated to test

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.deleteSmsOnSim() for the response returned.
 */
TEST_F(RadioHidlTest, deleteSmsOnSim) {
    int serial = 0;
    int index = 1;

    radio->deleteSmsOnSim(++serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NO_SUCH_ENTRY);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.sendSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendCdmaSms) {
    int serial = 0;

    //create a CdmaSmsAddress
    android::hardware::radio::V1_0::CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = android::hardware::radio::V1_0::CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = android::hardware::radio::V1_0::CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = android::hardware::radio::V1_0::CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = android::hardware::radio::V1_0::CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>) {11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3}; 

    //create a CdmaSmsSubAddress
    android::hardware::radio::V1_0::CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = android::hardware::radio::V1_0::CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>) {};

    //create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData = (std::vector<uint8_t>) 
	{15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio->sendCdmaSms(++serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_STATE);

    // TODO : radioRsp->sendSmsResult needs to be investigated to test

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.acknowledgeLastIncomingCdmaSms() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeLastIncomingCdmaSms) {
    int serial = 0;

    // Create a CdmaSmsAck
    android::hardware::radio::V1_0::CdmaSmsAck cdmaSmsAck;
    cdmaSmsAck.errorClass = android::hardware::radio::V1_0::CdmaSmsErrorClass::NO_ERROR;
    cdmaSmsAck.smsCauseCode = 1;

    radio->acknowledgeLastIncomingCdmaSms(++serial, cdmaSmsAck);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NO_SMS_TO_ACK);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.writeSmsToRuim() for the response returned.
 */
TEST_F(RadioHidlTest, writeSmsToRuim) {
    int serial = 0;

    //create a CdmaSmsAddress
    android::hardware::radio::V1_0::CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = android::hardware::radio::V1_0::CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = android::hardware::radio::V1_0::CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = android::hardware::radio::V1_0::CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = android::hardware::radio::V1_0::CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>) {11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3}; 

    //create a CdmaSmsSubAddress
    android::hardware::radio::V1_0::CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = android::hardware::radio::V1_0::CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>) {};

    //create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData = (std::vector<uint8_t>) 
	{15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    //create a CdmaSmsWriteArgs
    android::hardware::radio::V1_0::CdmaSmsWriteArgs cdmaSmsWriteArgs;
    cdmaSmsWriteArgs.status = android::hardware::radio::V1_0::CdmaSmsWriteArgsStatus::REC_UNREAD;
    cdmaSmsWriteArgs.message = cdmaSmsMessage;

    radio->writeSmsToRuim(++serial, cdmaSmsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::SYSTEM_ERR);

    // TODO : radioRsp->writeSmsToRuimIndex needs to be investigated to test

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.deleteSmsOnRuim() for the response returned.
 */
TEST_F(RadioHidlTest, deleteSmsOnRuim) {
    int serial = 0;
    int index = 1;

    radio->deleteSmsOnRuim(++serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_MODEM_STATE);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.getSmscAddress() for the response returned.
 */
TEST_F(RadioHidlTest, getSmscAddress) {
    int serial = 0;

    radio->getSmscAddress(++serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    // TODO : radioRsp->smscAddress needs to be investigated to test

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.setSmscAddress() for the response returned.
 */
TEST_F(RadioHidlTest, setSmscAddress) {
    int serial = 0;
    hidl_string address = hidl_string("smscAddress");

    radio->setSmscAddress(++serial, address);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_SMS_FORMAT);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.reportSmsMemoryStatus() for the response returned.
 */
TEST_F(RadioHidlTest, reportSmsMemoryStatus) {
    int serial = 0;
    bool available = true;

    radio->reportSmsMemoryStatus(++serial, available);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.acknowledgeIncomingGsmSmsWithPdu() for the response returned.
 */
TEST_F(RadioHidlTest, acknowledgeIncomingGsmSmsWithPdu) {
    int serial = 0;
    bool success = true;
    std::string ackPdu = "";

    radio->acknowledgeIncomingGsmSmsWithPdu(++serial, success, ackPdu);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::GENERIC_FAILURE);

    // TODO : Add tests related to error code returned
}

/*
 * Test IRadio.sendImsSms() for the response returned.
 */
TEST_F(RadioHidlTest, sendImsSms) {
    int serial = 1;
    
    //create a CdmaSmsAddress
    android::hardware::radio::V1_0::CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = android::hardware::radio::V1_0::CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = android::hardware::radio::V1_0::CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = android::hardware::radio::V1_0::CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = android::hardware::radio::V1_0::CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>) {11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3}; 

    //create a CdmaSmsSubAddress
    android::hardware::radio::V1_0::CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = android::hardware::radio::V1_0::CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>) {};

    //create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData = (std::vector<uint8_t>) 
	{15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    //creata an ImsSmsMessage
    android::hardware::radio::V1_0::ImsSmsMessage msg;
    msg.tech = android::hardware::radio::V1_0::RadioTechnologyFamily::THREE_GPP2;
    msg.retry = false;
    msg.messageRef = 0;
    msg.cdmaMessage = (std::vector<android::hardware::radio::V1_0::CdmaSmsMessage>) {cdmaSmsMessage};
    msg.gsmMessage = (std::vector<android::hardware::radio::V1_0::GsmSmsMessage>) {};

    radio->sendImsSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::INVALID_ARGUMENTS);

    // TODO : radioRsp->sendSmsResult needs to be investigated to test

    // TODO : Add tests related to error code returned

}
