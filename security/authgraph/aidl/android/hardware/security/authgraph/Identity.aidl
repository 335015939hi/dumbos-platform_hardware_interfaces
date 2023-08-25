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
 * Persistent (versioned) identity of a participant of Authgraph key exchange.
 * Identity consists of two main parts:
 *     1. certificate chain (e.g. a DICE certificate chain)
 *     2. a policy specifying how to verify the certificate chain - if a policy is not provided,
 *        a simple byte-to-byte comparison of the certificate chain is assumed.
 *
 * Persistent arcs created in Authgraph are associated with the participants' policies. The requests
 * to access those arcs are associated with the latest identity. During identity verification, the
 * certificate chain attached to the access request is compared against the policy attached to the
 * persistent arcs.
 *
 * The usage of policy based identity verification in Authgraph is three-fold:
 *        1. Retain access to persistent arcs from the newer versions of the same party who created
 *           them, even when parts of the cerificate chain are updated in the new version.
 *        2. Deny access to the new persistent arcs from the older versions of the same party who
 *           created the new persistent arcs, by including an updated policy in the identity.
 *        3. Trigger rotation of critical keys encrypted in persistent arcs created by the previous
 *           version of the party, by including an updated policy in the identity.
 */
@VintfStability
parcelable Identity {
    /* See Arc.cddl for the CDDL of `CertChain`*/
    byte[] certChain;

    /* See Arc.cddl for the CDDL of `Policy`*/
    @nullable byte[] policy;
}
