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

/**
 * Contains Galileo ionospheric model.
 * TODO: Defined in Galileo OS SIS ICD v2.1, 5.1.6?
 */
@VintfStability
parcelable GalileoIonosphericModel {
    double ai0;
    double ai1;
    double ai2;
    long transmissionTime;
}

/**
 * Contains Klobuchar ionospheric model used by GPS, BDS, QZSS.
 * TODO: Defined in ?
 */
@VintfStability
parcelable KlobucharIonosphericModel {
    /**
    * Klobuchar cefficients broadcast in satellite navigation message needed
    * to convert into correction parameters ? TODO: do we need this comment?
    */
    double alpha0;
    double alpha1;
    double alpha2;
    double alpha3;
    double beta0;
    double beta1;
    double beta2;
    double beta3;
    long transmissionTime;
}