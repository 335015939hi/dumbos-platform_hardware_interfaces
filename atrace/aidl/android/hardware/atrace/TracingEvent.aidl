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

package android.hardware.atrace;

@VintfStability
parcelable TracingEvent {
  /**
   * Name of the ftrace event group.
   * For example the sched/sched_switch event defined at
   * /sys/kernel/debug/tracing/events/sched/sched_switch
   * has the group "sched".
   */
  String group;

  /**
   * Name of the ftrace event.
   * For example the sched/sched_switch event defined at
   * /sys/kernel/debug/tracing/events/sched/sched_switch
   * has the name "sched_switch".
   *
   * If this event enables a whole group of ftrace events the name must be "*".
   */
  String name;

  /**
   * If true this event a hard requirement for enabling this category.
   * If this is true then attempting to enable a category containing this event
   * will fail if enabling the event is not successful this will leave the HAL
   * in a state where nothing is enabled.
   */
  boolean required;
}

