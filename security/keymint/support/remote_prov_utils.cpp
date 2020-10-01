/*
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <remote_prov/remote_prov_utils.h>

#include <openssl/rand.h>

#include <cppbor.h>

namespace aidl::android::hardware::security::keymint::remote_prov {

bytevec kTestMacKey(32 /* count */, 0 /* byte value */);

bytevec randomBytes(size_t numBytes) {
    bytevec retval(numBytes);
    RAND_bytes(retval.data(), numBytes);
    return retval;
}

ErrMsgOr<EekChain> generateEekChain(size_t length, const bytevec& eekId) {
    auto eekChain = cppbor::Array();

    bytevec prev_priv_key;
    for (size_t i = 0; i < length - 1; ++i) {
        bytevec pub_key(ED25519_PUBLIC_KEY_LEN);
        bytevec priv_key(ED25519_PRIVATE_KEY_LEN);

        ED25519_keypair(pub_key.data(), priv_key.data());

        // The first signing key is self-signed.
        if (prev_priv_key.empty()) prev_priv_key = priv_key;

        auto coseSign1 = constructCoseSign1(prev_priv_key,
                                            cppbor::Map() /* payload CoseKey */
                                                    .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                                                    .add(CoseKey::ALGORITHM, EDDSA)
                                                    .add(CoseKey::CURVE, ED25519)
                                                    .add(CoseKey::PUBKEY_X, pub_key)
                                                    .canonicalize()
                                                    .encode(),
                                            {} /* AAD */);
        if (!coseSign1) return coseSign1.moveMessage();
        eekChain.add(coseSign1.moveValue());
    }

    bytevec pub_key(X25519_PUBLIC_VALUE_LEN);
    bytevec priv_key(X25519_PRIVATE_KEY_LEN);
    X25519_keypair(pub_key.data(), priv_key.data());

    auto coseSign1 = constructCoseSign1(prev_priv_key,
                                        cppbor::Map() /* payload CoseKey */
                                                .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                                                .add(CoseKey::KEY_ID, eekId)
                                                .add(CoseKey::ALGORITHM, ECDH_ES_HKDF_256)
                                                .add(CoseKey::CURVE, cppcose::X25519)
                                                .add(CoseKey::PUBKEY_X, pub_key)
                                                .canonicalize()
                                                .encode(),
                                        {} /* AAD */);
    if (!coseSign1) return coseSign1.moveMessage();
    eekChain.add(coseSign1.moveValue());

    return EekChain{eekChain.encode(), pub_key, priv_key};
}

ErrMsgOr<std::vector<BccEntryData>> validateBcc(const cppbor::Array* bcc) {
    if (!bcc || bcc->size() == 0) return "Invalid BCC";

    std::vector<BccEntryData> result;

    bytevec prevKey;
    for (size_t i = 0; i < bcc->size(); ++i) {
        const cppbor::Array* entry = bcc->get(i)->asArray();
        if (!entry || entry->size() != kCoseSign1EntryCount) {
            return "Invalid BCC entry " + std::to_string(i) + ": " + prettyPrint(entry);
        }

        auto payload = verifyAndParseCoseSign1(false /* ignoreSignature */, entry,
                                               std::move(prevKey), bytevec{} /* AAD */);
        if (!payload) {
            return "Failed to verify entry " + std::to_string(i) + ": " + payload.moveMessage();
        }

        auto& certProtParms = entry->get(kCoseSign1ProtectedParams);
        if (!certProtParms || !certProtParms->asBstr()) return "Invalid prot params";
        auto [parsedProtParms, _, errMsg] = cppbor::parse(certProtParms->asBstr()->value());
        if (!parsedProtParms || !parsedProtParms->asMap()) return "Invalid prot params";

        bytevec firmwareDigest;
        auto& digestBstr = parsedProtParms->asMap()->get(-70001);
        if (digestBstr && digestBstr->asBstr()) firmwareDigest = digestBstr->asBstr()->value();

        result.push_back(BccEntryData{*payload, std::move(firmwareDigest)});

        // This entry's public key is the signing key for the next entry.
        prevKey = payload.moveValue();
    }

    return result;
}

}  // namespace aidl::android::hardware::security::keymint::remote_prov
