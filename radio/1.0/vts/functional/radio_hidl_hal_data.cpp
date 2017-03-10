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
 * Test IRadio.getDataRegistrationState() for the response returned.
 */
TEST_F(RadioHidlTest, getDataRegistrationState) {
    int serial = 0;

    radio->getDataRegistrationState(++serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setupDataCall() for the response returned.
 */
TEST_F(RadioHidlTest, setupDataCall) {
    int serial = 0;
    RadioTechnology radioTechnology = RadioTechnology::UNKNOWN;
    DataProfileInfo dataProfileInfo;
    bool modemCognitive = false;
    bool roamingAllowed = false;
    bool isRoaming = false;

    radio->setupDataCall(++serial, radioTechnology, dataProfileInfo, modemCognitive,
roamingAllowed, isRoaming);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.deactivateDataCall() for the response returned.
 */
TEST_F(RadioHidlTest, deactivateDataCall) {
    int serial = 0;
    int cid = 1;
    bool reasonRadioShutDown = false;

    radio->deactivateDataCall(++serial, cid, reasonRadioShutDown);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::INVALID_CALL_ID, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getDataCallList() for the response returned.
 */
TEST_F(RadioHidlTest, getDataCallList) {
    int serial = 0;

    radio->getDataCallList(++serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::NONE
                || radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS);
    }
}

/*
 * Test IRadio.setInitialAttachApn() for the response returned.
 */
TEST_F(RadioHidlTest, setInitialAttachApn) {
    int serial = 0;
    DataProfileInfo dataProfileInfo;

    bool modemCognitive = false;
    bool isRoaming = false;

    radio->setInitialAttachApn(++serial, dataProfileInfo,
            modemCognitive, isRoaming);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setDataAllowed() for the response returned.
 */
TEST_F(RadioHidlTest, setDataAllowed) {
    int serial = 0;
    bool allow = true;

    radio->setDataAllowed(++serial, allow);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.setDataProfile() for the response returned.
 */
TEST_F(RadioHidlTest, setDataProfile) {
    int serial = 0;
    android::hardware::hidl_vec<DataProfileInfo> profiles;
    memset(&profiles, 0, sizeof(profiles));
    bool isRoadming = false;

    radio->setDataProfile(++serial, profiles, isRoadming);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::INVALID_STATE, radioRsp->rspInfo.error);
    }
}

