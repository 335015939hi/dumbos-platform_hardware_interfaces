/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.power;

@VintfStability
@Backing(type="int")
enum Boost {
   /**
    * This boost is set when user interacting with the device, for example,
    * touchscreen events are incoming. CPU and GPU load may be expected soon,
    * and it may be appropriate to raise speeds of CPU, memory bus etc.
    * Note that this is different from INTERACTIVCE mode, which only indicates
    * that such interaction *may* occur, not that it is actively occurring. Since
    * when the user is interacting with the device, it is likely that the CPU and
    * GPU load may increase, so it may be appropriate to raise the frequencies
    * of the CPU, GPU, memory buses, etc.
    */
    INTERACTION,

   /**
    * This boost indicates that the device is interacting with ML  acceleraror.
    * Currently not used in Android framework but can be used for device specific
    * optimizations for ML workload.
    */
    ML_ACC,

   /**
    * This boost indicates that the device is setting up audio stream.
    * Currently not used in Android framework but can be used for device
    * specific optimizations during setting up audio stream.
    */
    AUDIO_LAUNCH,

  /**
    * This boost indicates that camera is being launched.
    * Currently not used in Android framework but can be used
    * for device specific performance optimizations on starting camera.
    */
    CAMERA_LAUNCH,

   /**
    * This boost indicates that camera secure stream is being started.
    * Currently not used in Android framework but can be used for device
    * specific optimizations during taking camera shot.
    */
    CAMERA_SHOT,
}
