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

package android.hardware.wifi.hostapd;

/**
 * The channel bandwidth of the AP.
 */
@VintfStability
@Backing(type="int")
enum Bandwidth {
    WIFI_BANDWIDTH_INVALID = 0,
    WIFI_BANDWIDTH_20_NOHT = 1,
    WIFI_BANDWIDTH_20 = 2,
    WIFI_BANDWIDTH_40 = 3,
    WIFI_BANDWIDTH_80 = 4,
    WIFI_BANDWIDTH_80P80 = 5,
    WIFI_BANDWIDTH_160 = 6,
    WIFI_BANDWIDTH_2160 = 7,
    WIFI_BANDWIDTH_4320 = 8,
    WIFI_BANDWIDTH_6480 = 9,
    WIFI_BANDWIDTH_8640 = 10,
}
