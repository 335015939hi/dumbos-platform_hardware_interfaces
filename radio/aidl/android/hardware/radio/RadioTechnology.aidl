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

package android.hardware.radio;

@VintfStability
@Backing(type="int")
enum RadioTechnology {
    UNKNOWN = 0,
    GPRS = 1,
    EDGE = 2,
    UMTS = 3,
    IS95A = 4,
    IS95B = 5,
    ONE_X_RTT = 6,
    EVDO_0 = 7,
    EVDO_A = 8,
    HSDPA = 9,
    HSUPA = 10,
    HSPA = 11,
    EVDO_B = 12,
    EHRPD = 13,
    LTE = 14,
    /**
     * HSPA+
     */
    HSPAP = 15,
    /**
     * Only supports voice
     */
    GSM = 16,
    TD_SCDMA = 17,
    IWLAN = 18,
    LTE_CA = 19,
    /**
     * 5G NR. This is only use in 5G Standalone mode.
     */
    NR = 20,
}
