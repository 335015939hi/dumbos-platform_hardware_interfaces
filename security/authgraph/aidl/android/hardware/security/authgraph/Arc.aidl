/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

/**
 * This is the definition of the data format of an Arc.
 * @hide
 */
@VintfStability
parcelable Arc {
    /**
     * The messages exchanged between the domains are called Arcs. An arc is simply AES-GCM
     * encryption of a payload P with a key K and additional authentication data (AAD) D
     * (i.e. Arc = Enc(K, P, D)).
     * The CDDL of an arc is as follows.
     * Arc = = [ // COSE_Encrypt0
     *     protected : bstr .cbor {
     *         1 : 3		      // alg: A256GCM
     *         -70000 : arc_type      // arc_type: any of the defined arc_types
     *         ? -70001 : [+permission] // permissions: one or more from permission
     *         ? -70002 : [+limitation]// limitations: one or more from limitations
     *         -70003 : int/tstr,     // current timestamp
     *     }
     *     unprotected : {
     *         4 : bstr .cbor SourceNode, // kid: hint about the  source node of the arc
     *         5 : bstr .size 12          // IV
     *     }
     *     ciphertext : bstr    // Enc(K, DestinationNode/Arc, encoded ArcEncStruct)
     * ]
     */
    byte[] Arc;
}
