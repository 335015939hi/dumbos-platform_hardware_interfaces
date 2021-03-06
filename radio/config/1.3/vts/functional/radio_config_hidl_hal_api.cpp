/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <radio_config_hidl_hal_utils.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadioConfig.getHalDeviceCapabilities()
 */
TEST_P(RadioConfigHidlTest, getHalDeviceCapabilities) {
    const int serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getHalDeviceCapabilities(serial);
    ASSERT_OK(res);
    ALOGI("getHalDeviceCapabilities, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());
}

/*
 * Test IRadioConfig.setPreferredDataModem_1_3()
 */
TEST_P(RadioConfigHidlTest, setPreferredDataModem_1_3) {
    serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getPhoneCapability(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getPhoneCapability, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioConfigRsp->rspInfo.error != RadioError ::NONE) {
        return;
    }

    if (radioConfigRsp->phoneCap.logicalModemList.size() == 0) {
        return;
    }

    // We get phoneCapability. send setPreferredDataModem command
    serial = GetRandomSerialNumber();
    uint8_t modemId = radioConfigRsp->phoneCap.logicalModemList[0].modemId;
    res = radioConfig->setPreferredDataModem_1_3(serial, modemId);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setPreferredDataModem_1_3, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::OP_NOT_ALLOWED_DURING_VOICE_CALL, RadioError::INVALID_SIM_STATE}));
}
