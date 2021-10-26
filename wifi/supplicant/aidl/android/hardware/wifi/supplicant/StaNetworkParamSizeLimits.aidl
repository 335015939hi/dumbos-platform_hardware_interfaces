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

package android.hardware.wifi.supplicant;

/**
 * Size limits for some of the params used in this interface.
 */
@VintfStability
@Backing(type="int")
enum StaNetworkParamSizeLimits {
    /**
     * Max length of SSID param.
     */
    SSID_MAX_LEN_IN_BYTES = 32,
    /**
     * Min length of PSK passphrase param.
     */
    PSK_PASSPHRASE_MIN_LEN_IN_BYTES = 8,
    /**
     * Max length of PSK passphrase param.
     */
    PSK_PASSPHRASE_MAX_LEN_IN_BYTES = 63,
    /**
     * Max number of WEP keys param.
     */
    WEP_KEYS_MAX_NUM = 4,
    /**
     * Length of each WEP40 keys param.
     */
    WEP40_KEY_LEN_IN_BYTES = 5,
    /**
     * Length of each WEP104 keys param.
     */
    WEP104_KEY_LEN_IN_BYTES = 13,
}
