/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_4.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT, .interval = 60, .specifiers = {specifier}};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan, rspInfo.error = %s\n", toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        // REQUEST_NOT_SUPPORTED should not be allowed as it is not an optional API. This might be
        // allowed in future because some vendors do not support this request on dual sim devices.
        // OPERATION_NOT_ALLOWED should not be allowed; however, some vendors do not support the
        // required manual GSM search functionality. This is tracked in b/112206766.
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::NONE, RadioError::OPERATION_NOT_ALLOWED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid specifier.
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidArgument) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT, .interval = 60};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidInterval1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 4,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidInterval2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 301,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidMaxSearchTime1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 59,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidMaxSearchTime2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 3601,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidPeriodicity1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 0};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidPeriodicity2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 11};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_GoodRequest1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            // Some vendor may not support max search time of 360s.
            // This issue is tracked in b/112205669.
            .maxSearchTime = 300,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 10};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_4->rspInfo.error, {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_4->rspInfo.error, {RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity and plmns
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_GoodRequest2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
            .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
            .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            // Some vendor may not support max search time of 360s.
            // This issue is tracked in b/112205669.
            .maxSearchTime = 300,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 10,
            .mccMncs = {"310410"}};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
            toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_4->rspInfo.error, {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_4->rspInfo.error, {RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }
}