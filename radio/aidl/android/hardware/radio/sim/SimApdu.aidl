<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
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

package android.hardware.radio.sim;

@VintfStability
@JavaDerive(toString=true)
parcelable SimApdu {
    /**
     * "sessionid" from TS 27.007 +CGLA command. Must be ignored for +CSIM command.
     */
    int sessionId;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int cla;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int instruction;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int p1;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int p2;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     * A negative P3 implies a 4 byte APDU.
     */
    int p3;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     * In hex string format ([a-fA-F0-9]*)
     */
    String data;
}
=======
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

package android.hardware.radio.sim;

@VintfStability
@JavaDerive(toString=true)
parcelable SimApdu {
    /**
     * "sessionid" from TS 27.007 +CGLA command. Must be ignored for +CSIM command.
     */
    int sessionId;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int cla;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int instruction;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int p1;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     */
    int p2;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     * A negative P3 implies a 4 byte APDU.
     */
    int p3;
    /**
     * Used to derive the APDU ("command" and "length" values in TS 27.007 +CSIM and +CGLA commands)
     * In hex string format ([a-fA-F0-9]*)
     */
    String data;
    /**
     * isEs10 indicates that the current streaming APDU contains an ES10 command or it is a regular
     * APDU. (As per spec SGP.22 V3.0, ES10 commands needs to be sent over command port of MEP-A1)
     */
    boolean isEs10 = false;
}
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
