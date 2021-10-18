/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.bluetooth.audio3;

import android.hardware.bluetooth.audio3.Lc3Parameters;

/**
 * Used to configure a LC3 Hardware Encoding session.
 */
@VintfStability
parcelable Lc3CodecConfiguration {
    /* The connection handle used for the unicast or the broadcast group. The minimum size should
     * be 1. The handle order should corresponds the configured bit order of the audio channel
     * allocation */
    int[] handle;
    /*
     * This is also bitfield, specifying how the channels are ordered in the outgoing media packet.
     * Bit meaning is defined in Bluetooth Assigned Numbers.
     */
    int audioChannelAllocation;
    Lc3Parameters lc3Config;
}
