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

import android.hardware.gnss.gnss_assistance.DgnssCorrectionComponent;

/**
 * Contains differential GNSS signal correction.
 *
 */
@VintfStability
parcelable DgnssSignalCorrection {
    /**
    * Differentiates signals from the same satellite, e.g. GPS L1/L5
    */
    long carrierFrequencyHz;

    /**
     * The GNSS signal's code type. For OSR corrections, this is the code type
     * observed by the reference station. For SSR corrections, the code type is
     * optional. If present, it reflects what was used during computation of the
     * correction though, in practice, does not affect the output and may be
     * ignored.
     * See:
     * https://developer.android.com/reference/android/location/GnssMeasurement.html#getCodeType()
     * TODO: Change type to GnssSignalType ?
     */
    String codeType;

    /**
    * State Space Representation providing estimates of the different error
    * components. In the future, this message may be extended to support
    * clock/orbit components.
    * TODO: Rename to DgnssSsrCorrection ?
    */
    @VintfStability
    parcelable SsrCorrection {
        /**
        * Ionospheric component of total error.
        */
        DgnssCorrectionComponent ionosphere;

        /**
        * Tropospheric component of total error.
        */
        DgnssCorrectionComponent troposphere;
    }

    /**
     * State Space Representation providing estimates of the different error
     * components.
     */
    SsrCorrection ssr;

    /**
    * Observation Space Representation providing the sum of all errors observed
    * by a reference station.
    * TODO: Rename to DgnssOsrCorrection ?
    */
    @VintfStability
    parcelable OsrCorrection {
        /**
        * Sum of all errors observed by a reference station.
        */
        DgnssCorrectionComponent pathDelay;
    }

    /**
     * Observation Space Representation providing the sum of all errors observed
     * by a reference station. Each element reports the OSR corrections from a
     * single reference station. Empty indicates that there weren't any available
     * reference stations near the receiver. When multiple reference stations
     * are nearby, the client may select the preferred station.
     */
    OsrCorrection[] osr;
}
