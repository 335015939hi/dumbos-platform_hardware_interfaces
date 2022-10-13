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

package android.hardware.audio.effect;

/**
 * Some common capability for an effect instance.
 */
@VintfStability
parcelable Flags {
    // Connection Mode.
    @VintfStability
    @Backing(type="byte")
    enum Type {
        // after track process.
        INSERT = 0,
        // auxiliary: connect to track auxiliary output and use send level.
        AUXILIARY = 1,
        // replaces track process function; must implement SRC, volume and mono to stereo.
        REPLACE = 2,
        // applied below audio HAL on in.
        PRE_PROC = 3,
        // applied below audio HAL on out.
        POST_PROC = 4,
    }
    Type type = Type.INSERT;

    // Insertion preference.
    @VintfStability
    @Backing(type="byte")
    enum Insert {
        ANY = 0,
        FIRST = 1, // first of the chain.
        LAST = 2, // last of the chain.
        EXCLUSIVE = 3, // exclusive (only effect in the insert chain).
    }
    Insert insert = Insert.ANY;

    @VintfStability
    @Backing(type="byte")
    enum Volume {
        NONE = 0,
        CTRL = 1, // implements volume control.
        IND = 2, // requires volume indication.
        MONITOR = 3, // monitors requested volume.
    }
    Volume volume = Volume.NONE;

    @VintfStability
    @Backing(type="byte")
    enum HardwareAccelerator {
        // No hardware acceleration
        NONE = 0,
        /**
         * non tunneled hw acceleration: effect reads the samples, send them to HW accelerated
         * effect processor, reads back the processed samples and returns them to the output buffer.
         */
        SIMPLE = 1,
        /**
         * The effect interface is only used to control the effect engine.
          This mode is relevant for global effects actually applied by the audio hardware on the
         output stream.
         */
        TUNNEL = 2,
    }
    HardwareAccelerator hwAcceleratorMode = HardwareAccelerator.NONE;

    // if offload mode is supported.
    boolean offloadSupported;

    // support device updates if it's true.
    boolean deviceIndication;

    // support audio mode updates if it's true.
    boolean audioModeIndication;

    // support audio source updates if it's true.
    boolean audioSourceIndication;

    // true if no processing done for this effect instance.
    boolean noProcess;
}
