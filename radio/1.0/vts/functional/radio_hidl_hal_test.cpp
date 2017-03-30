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

#include <radio_hidl_hal_utils.h>

void RadioHidlTest::SetUp() {
  radio = ::testing::VtsHalHidlTargetTestBase::getService<IRadio>(
      hidl_string("rild"));
  ASSERT_NE(radio, nullptr);

  radioRsp = new RadioResponse();
  ASSERT_NE(radioRsp, nullptr);

  radioInd = NULL;
  radio->setResponseFunctions(radioRsp, radioInd);

  radio->getIccCardStatus(1);
  auto res = radioRsp->WaitForCallback();
  EXPECT_TRUE(res.first);
  auto rspInfo = res.second;
  EXPECT_EQ(RadioResponseType::SOLICITED, rspInfo->type);
  EXPECT_EQ(1, rspInfo->serial);
  EXPECT_EQ(RadioError::NONE, rspInfo->error);
}

void RadioHidlTest::TearDown() {}
