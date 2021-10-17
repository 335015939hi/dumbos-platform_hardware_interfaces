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

package android.hardware.bluetooth.audio3;

@VintfStability
@Backing(type="long")
enum AudioContentType {
    // Do not change these values without updating their counterparts
    // in frameworks/base/media/java/android/media/AudioAttributes.java
    UNKNOWN = 0,
    SPEECH = 1,
    MUSIC = 2,
    MOVIE = 3,
    SONIFICATION = 4,
}
