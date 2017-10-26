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

#include <radio_hidl_hal_utils_v1_2.h>
#include <vector>

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid specifier.
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidArgument) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidInterval1) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 4;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 60;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 1;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidInterval2) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 301;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 60;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 1;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidMaxSearchTime1) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 59;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 1;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidMaxSearchTime2) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 3601;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 1;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidPeriodicity1) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 600;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 0;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidPeriodicity2) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 600;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 11;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %d\n",
          static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_GoodRequest1) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 600;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 10;

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %d\n",
              static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity and plmns
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_GoodRequest2) {
    const int serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {};
    request.type = ScanType::PERIODIC;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;
    request.maxSearchTime = 600;
    request.incrementalResults = false;
    request.incrementalResultsPeriodicity = 10;
    request.mccMncs.resize(1);
    request.mccMncs[0] = "310410";

    radio_v1_2->startNetworkScan_1_2(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %d\n",
              static_cast<int32_t>(radioRsp_v1_2->rspInfo.error));
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_2->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(radioRsp_v1_2->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_2->rspInfo.error == RadioError::OPERATION_NOT_ALLOWED ||
                    radioRsp_v1_2->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}
