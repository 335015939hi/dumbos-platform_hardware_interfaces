/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.earc@1.0-service"

#include <android/hardware/tv/earc/1.0/HdmiEarc.h>
#include <hidl/HidlTransportSupport.h>

using android::OK;
using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::tv::earc::V1_0::IHdmiEarc;
using android::hardware::tv::earc::V1_0::implementation::HdmiEarc;

int main() {
    configureRpcThreadpool(10, true);

    sp<IHdmiEarc> myHdmiEarc = new HdmiEarc();
    if (myHdmiEarc->registerAsService() != OK) {
        ALOGE("Could not register HdmiEarc as service");
        return 1;
    }
    ALOGE("register HdmiEarc as service successfully");
    joinRpcThreadpool();
    return 0;
}
