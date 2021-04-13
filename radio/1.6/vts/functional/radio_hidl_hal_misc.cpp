/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <regex>

#include <android-base/logging.h>
#include <radio_hidl_hal_utils_v1_6.h>

/*
 * Test IRadio.getAvailableNetworks() for the response returned.
 */
TEST_P(RadioHidlTest, getAvailableNetworks) {
    LOG(DEBUG) << "getAvailableNetworks";
    serial = GetRandomSerialNumber();
    radio->getAvailableNetworks(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    ASSERT_TRUE(radioRsp->rspInfo.type == RadioResponseType::SOLICITED ||
                radioRsp->rspInfo.type == RadioResponseType::SOLICITED_ACK_EXP);
    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::CANCELLED, RadioError::DEVICE_IN_USE,
                              RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                             CHECK_GENERAL_ERROR));
    }
    static const std::regex kOperatorNumericRe("^[0-9]{5,6}$");
    for (OperatorInfo info : radioRsp->networkInfos) {
        if (info.operatorNumeric != nullptr) {
            ASSERT_TRUE(std::regex_match(std::string(info.operatorNumeric), kOperatorNumericRe));
        }
    }
    LOG(DEBUG) << "getAvailableNetworks finished";
}
