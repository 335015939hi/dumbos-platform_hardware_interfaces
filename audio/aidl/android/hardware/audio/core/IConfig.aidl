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

import android.media.audio.common.AudioHalEngineConfig;
/**
 * This interface provides system-wide configuration parameters for audio I/O
 * (by "system" here we mean the device running Android).
 */
@VintfStability
interface IConfig {
    /**
     * Returns the configuration items used to determine the audio policy engine
     * flavor and initial configuration.
     *
     * Engine flavor is determined by presence of capSpecificConfig, which must
     * only be present if vendors are using the Configurable Audio Policy (CAP)
     * engine. Vendors wanting to use the default audio policy engine
     * implementation must indicate this by setting capSpecificConfig to an
     * optional type that does not contain a value (e.g. std::nullopt).
     *
     * This method will only be called on initial loading of the audio policy
     * engine, and must always return the same result.
     *
     * @return The engine configuration
     */
    AudioHalEngineConfig getEngineConfig();
}
