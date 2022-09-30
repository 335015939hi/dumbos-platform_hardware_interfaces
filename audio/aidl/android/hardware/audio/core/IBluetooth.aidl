/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.audio.core;

/**
 * An instance of IBluetooth manages settings for the Hands-Free Profile (HFP)
 * and the SCO Link. This interface is optional to implement and provide by the
 * vendor. It needs to be provided only if the device actually supports BT SCO
 * or HFP.
 */
@VintfStability
interface IBluetooth {
    @JavaDerive(equals=true, toString=true)
    @VintfStability
    parcelable ScoConfig {
        /**
         * Whether BT SCO Noise Reduction and Echo Cancellation are enabled.
         */
        boolean isNrecEnabled;
        /**
         * Whether BT SCO Wideband mode is enabled.
         */
        boolean isWidebandEnabled;
        /**
         * The name of the BT SCO headset used for debugging purposes.
         */
        @utf8InCpp String debugName;
    }

    /**
     * Retrieve the configuration of Bluetooth SCO.
     *
     * The SCO functionality may be disabled by the client. In that case
     * a null value is returned.
     *
     * @return The current configuration.
     * @throws EX_UNSUPPORTED_OPERATION If BT SCO is not supported.
     */
    @nullable ScoConfig getScoConfig();
    /**
     * Set the configuration of Bluetooth SCO.
     *
     * The SCO functionality may be disabled by the client. The client
     * send a null value for the config in order to do that.
     *
     * @param config The configuration to set, or a null value.
     * @throws EX_UNSUPPORTED_OPERATION If BT SCO is not supported.
     */
    void setScoConfig(in @nullable ScoConfig config);

    @JavaDerive(equals=true, toString=true)
    @VintfStability
    parcelable HfpConfig {
        /**
         * The sample rate of BT HFP, in Hertz.
         */
        int sampleRate;
        /**
         * The output volume of BT HFP. 1.0f means unity gain, 0.0f is muted.
         */
        float volume;
    }

    /**
     * Retrieve the configuration of Bluetooth HFP.
     *
     * The HFP functionality may be disabled by the client. In that case
     * a null value is returned.
     *
     * @return The current configuration or null.
     * @throws EX_UNSUPPORTED_OPERATION If BT HFP is not supported.
     */
    @nullable HfpConfig getHfpConfig();
    /**
     * Set the configuration of Bluetooth HFP.
     *
     * The HFP functionality may be disabled by the client. The client
     * send a null value for the config in order to do that.
     *
     * @param config The configuration to set, or a null value.
     * @throws EX_UNSUPPORTED_OPERATION If BT HFP is not supported.
     */
    void setHfpConfig(in @nullable HfpConfig config);
}
