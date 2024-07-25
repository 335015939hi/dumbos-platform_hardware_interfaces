/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.GnssConstellationType;

/*
 * Contains the GNSS-GNSS system time offset between the GNSS system time.
 * TODO: Defined in ?
 */
@VintfStability
parcelable TimeModel {
  /*
   * Model represents parameters to convert from current GNSS to GNSS system
   * time indicated by to_gnss.
   *
   * TODO: This is different from the type in google3/java/com/google/location/lbs/supl/data/proto/gnss_type.proto
   */
  GnssConstellationType to_gnss;

  /*
   * Coefficients A0 and A1 are used together to calculate the time
   * correction needed.
   * a0, a1 Coefficients of 1-deg polynomial (a0 sec, a1 sec/sec)
   * CORR(s) = a0 + a1 * DELTAT
   * GLONASS - a0 = TauC, a1=zero
   */
  double a0;
  double a1;

  /** Reference time of week in GNSS system time. */
  int timeOfWeek; // Renamed for better readability

  /** Reference week number in GNSS system time. */
  int weekNumber;

  long transmissionTime;
}
