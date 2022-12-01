/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <android/binder_manager.h>

#include "radio_sap_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define TIMEOUT_PERIOD 40

void RadioSapTest::SetUp() {
    sap = ISap::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(sap.get(), nullptr);

    sapCb = ndk::SharedRefBase::make<RadioSapResponse>(*this);
    ASSERT_NE(sapCb.get(), nullptr);

    count = 0;

    ndk::ScopedAStatus res = sap->setCallback(sapCb);
    ASSERT_OK(res);
}

void RadioSapTest::TearDown() {}

int RadioSapTest::GetRandomToken() {
    return rand();
}

::testing::AssertionResult RadioSapTest::CheckAnyOfErrors(SapResultCode err,
                                                          std::vector<SapResultCode> errors) {
    for (size_t i = 0; i < errors.size(); i++) {
        if (err == errors[i]) {
            return testing::AssertionSuccess();
        }
    }
    return testing::AssertionFailure() << "SapError:" + toString(err) + " is returned";
}

void RadioSapTest::notify(int receivedToken) {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    if (token == receivedToken) {
        cv.notify_one();
    }
}

std::cv_status RadioSapTest::wait() {
    std::unique_lock<std::mutex> lock(mtx);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count == 0) {
        status = cv.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        if (status == std::cv_status::timeout) {
            return status;
        }
    }
    count--;
    return status;
}

/*
 * Test ISap.connectReq() for the response returned.
 */
TEST_P(RadioSapTest, connectReq) {
    LOG(DEBUG) << "connectReq";
    token = GetRandomToken();
    int32_t maxMsgSize = 100;

    ndk::ScopedAStatus res = sap->connectReq(token, maxMsgSize);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    // Modem side need time for connect to finish. Adding a waiting time to prevent
    // disconnect being requested right after connect request.
    sleep(1);
}

/*
 * Test IRadio.disconnectReq() for the response returned
 */
TEST_P(RadioSapTest, disconnectReq) {
    LOG(DEBUG) << "disconnectReq";
    token = GetRandomToken();

    sap->disconnectReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);
    LOG(DEBUG) << "disconnectReq finished";
}

/*
 * Test IRadio.apduReq() for the response returned.
 */
TEST_P(RadioSapTest, apduReq) {
    LOG(DEBUG) << "apduReq";
    token = GetRandomSerialNumber();
    SapApduType sapApduType = SapApduType::APDU;
    std::vector<uint8_t> command = {};

    sap->apduReq(token, sapApduType, command);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(
            sapCb->sapResultCode,
            {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_ALREADY_POWERED_OFF,
             SapResultCode::CARD_NOT_ACCESSSIBLE, SapResultCode::CARD_REMOVED,
             SapResultCode::SUCCESS}));
    LOG(DEBUG) << "apduReq finished";
}

/*
 * Test IRadio.transferAtrReq() for the response returned.
 */
TEST_P(RadioSapTest, transferAtrReq) {
    LOG(DEBUG) << "transferAtrReq";
    token = GetRandomToken();

    sap->transferAtrReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::CARD_ALREADY_POWERED_OFF,
                                  SapResultCode::CARD_REMOVED, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "transferAtrReq finished";
}

/*
 * Test IRadio.powerReq() for the response returned.
 */
TEST_P(RadioSapTest, powerReq) {
    LOG(DEBUG) << "powerReq";
    token = GetRandomToken();
    bool state = true;

    sap->powerReq(token, state);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::CARD_ALREADY_POWERED_ON, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "powerReq finished";
}

/*
 * Test IRadio.resetSimReq() for the response returned.
 */
TEST_P(RadioSapTest, resetSimReq) {
    LOG(DEBUG) << "resetSimReq";
    token = GetRandomToken();

    sap->resetSimReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::SUCCESS}));
    LOG(DEBUG) << "resetSimReq finished";
}

/*
 * Test IRadio.transferCardReaderStatusReq() for the response returned.
 */
TEST_P(RadioSapTest, transferCardReaderStatusReq) {
    LOG(DEBUG) << "transferCardReaderStatusReq";
    token = GetRandomToken();

    sap->transferCardReaderStatusReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::SUCCESS}));
    LOG(DEBUG) << "transferCardReaderStatusReq finished";
}

/*
 * Test IRadio.setTransferProtocolReq() for the response returned.
 */
TEST_P(RadioSapTest, setTransferProtocolReq) {
    LOG(DEBUG) << "setTransferProtocolReq";
    token = GetRandomToken();
    SapTransferProtocol sapTransferProtocol = SapTransferProtocol::T0;

    sap->setTransferProtocolReq(token, sapTransferProtocol);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::NOT_SUPPORTED, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "setTransferProtocolReq finished";
}
