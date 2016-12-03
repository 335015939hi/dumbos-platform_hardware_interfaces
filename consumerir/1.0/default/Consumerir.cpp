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

#define LOG_TAG "ConsumerirService"
#include <android/log.h>

#include <hardware/hardware.h>
#include <hardware/consumerir.h>
#include "Consumerir.h"

namespace android {
namespace hardware {
namespace consumerir {
namespace V1_0 {
namespace implementation {

Consumerir::Consumerir(consumerir_device_t *device) {
    mDevice = device;
}

// Methods from ::android::hardware::consumerir::V1_0::IConsumerir follow.
Return<int32_t> Consumerir::transmit(int32_t carrierFreq, const hidl_vec<int32_t>& pattern, int32_t patternLen) {
    return mDevice->transmit(mDevice, carrierFreq, pattern.data(), patternLen);
}

Return<int32_t> Consumerir::getNumCarrierFreqs() {
    return mDevice->get_num_carrier_freqs(mDevice);
}

Return<void> Consumerir::getCarrierFreqs(uint64_t len, getCarrierFreqs_cb _hidl_cb) {
    consumerir_freq_range_t *rangeAr = new consumerir_freq_range_t[len];
    int32_t ret = mDevice->get_carrier_freqs(mDevice, len, rangeAr);
    hidl_vec<ConsumerirFreqRange> rangeVec;
    rangeVec.setToExternal((ConsumerirFreqRange*)rangeAr, len);
    _hidl_cb(ret, rangeVec);
    return Void();
}


IConsumerir* HIDL_FETCH_IConsumerir(const char *name) {
    consumerir_device_t *dev;
    const hw_module_t *hw_module = NULL;

    int ret = hw_get_module(name, &hw_module);
    if (ret != 0) {
        ALOGE("hw_get_module %s failed: %d", name, ret);
        return nullptr;
    }
    ret = hw_module->methods->open(hw_module, CONSUMERIR_TRANSMITTER, (hw_device_t **) &dev);
    if (ret < 0) {
        ALOGE("Can't open consumer IR transmitter, error: %d", ret);
        return nullptr;
    }
    return new Consumerir(dev);
}

} // namespace implementation
}  // namespace V1_0
}  // namespace consumerir
}  // namespace hardware
}  // namespace android
