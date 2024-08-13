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

package android.hardware.security.see.authmgr;

import android.hardware.security.see.authmgr.ClientDiceArtifactsForPhase2;
import android.hardware.security.see.authmgr.DicePolicy;
import android.hardware.security.see.authmgr.ExplicitKeyDiceCertChain;
import android.hardware.security.see.authmgr.SignedResponseForChallenge;

/**
 * This is the interface to be implemented by an AuthMgr backend component (AuthMgr BE), in order to
 * allow AuthMgr frontend components (AuthMgr FE) to authenticate themselves and authorize client
 * trusted applets (TA) to access the trusted services (i.e. Trusted HALs).
 *
 * At the end of the successful execution of the first two methods, the AuthMgr FE establishes an
 * authenticated connection with the AuthMgr BE with rollback protection. The AuthMgr FE invokes the
 * third methods of this API on such authenticated connection established with the AuthMgr BE, in
 * order to authorize a client TA to access a trusted service.
 *
 * Note that this interface defines a one-way authentication protocol from the AuthMgr FE to AuthMgr
 * BE. This does not cover mutual authentication.
 */
@VintfStability
interface IAuthMgrAuthorization {
    byte[32] initAuthentication(in ExplicitKeyDiceCertChain diceCertChain,
            in @nullable byte[64] instanceIdentifier);

    void completeAuthentication(
            in SignedResponseForChallenge SignedResponseForChallenge, in DicePolicy dicePolicy);

    void getTrustedServiceForClient(in byte[] clientID, String serviceName, in byte[32] token,
            in ClientDiceArtifactsForPhase2 clientDiceArtifacts);

    void deleteClient(in byte[] clientID);
}
