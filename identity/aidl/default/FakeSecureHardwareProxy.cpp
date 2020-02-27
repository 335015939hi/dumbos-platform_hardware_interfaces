/*
 * Copyright 2020, The Android Open Source Project
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


#define LOG_TAG "FakeSecureHardwareProxy"

#include "FakeSecureHardwareProxy.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <string.h>

#include <openssl/sha.h>

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hkdf.h>
#include <openssl/hmac.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "EmbeddedIc.h"

using ::std::optional;
using ::std::string;
using ::std::tuple;
using ::std::vector;

namespace android::hardware::identity {

// ----------------------------------------------------------------------

FakeSecureHardwareProvisioningProxy::FakeSecureHardwareProvisioningProxy() {
}

FakeSecureHardwareProvisioningProxy::~FakeSecureHardwareProvisioningProxy() {
}

bool FakeSecureHardwareProvisioningProxy::shutdown() {
  LOG(INFO) << "FakeSecureHardwarePresentationProxy shutdown";
  return true;
}

bool FakeSecureHardwareProvisioningProxy::initialize(bool testCredential) {
  LOG(INFO) << "FakeSecureHardwareProvisioningProxy created, sizeof(EicProvisioning): "
            << sizeof(EicProvisioning);
  return eicProvisioningInit(&ctx_, testCredential);
}

// Returns public key certificate.
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::createCredentialKey(
    const vector<uint8_t>& challenge,
    const vector<uint8_t>& applicationId) {
    uint8_t publicKeyCert[512];
    size_t publicKeyCertSize = sizeof publicKeyCert;
    if (!eicProvisioningCreateCredentialKey(&ctx_,
                                         challenge.data(), challenge.size(),
                                         applicationId.data(), applicationId.size(),
                                         publicKeyCert,
                                         &publicKeyCertSize)) {
        return {};
    }
    vector<uint8_t> pubKeyCert(publicKeyCertSize);
    memcpy(pubKeyCert.data(), publicKeyCert, publicKeyCertSize);
    return pubKeyCert;
}

bool FakeSecureHardwareProvisioningProxy::startPersonalization(
    int accessControlProfileCount,
    vector<int> entryCounts,
    string docType,
    size_t expectedProofOfProvisioningSize) {
    if (!eicProvisioningStartPersonalization(&ctx_, accessControlProfileCount,
                                          entryCounts.data(), entryCounts.size(),
                                          docType.c_str(),
                                          expectedProofOfProvisioningSize)) {
      return false;
    }
    return true;
}

// Returns MAC (28 bytes).
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::addAccessControlProfile(
    int id,
    vector<uint8_t> readerCertificate,
    bool userAuthenticationRequired,
    uint64_t timeoutMillis,
    uint64_t secureUserId) {
  vector<uint8_t> mac(28);
  if (!eicProvisioningAddAccessControlProfile(&ctx_, id,
                                           readerCertificate.data(), readerCertificate.size(),
                                           userAuthenticationRequired, timeoutMillis, secureUserId,
                                           mac.data())) {
    return {};
  }
  return mac;
}

bool FakeSecureHardwareProvisioningProxy::beginAddEntry(const vector<int>& accessControlProfileIds,
                                            string nameSpace, string name, uint64_t entrySize) {
  return eicProvisioningBeginAddEntry(&ctx_,
                                   accessControlProfileIds.data(), accessControlProfileIds.size(),
                                   nameSpace.c_str(), name.c_str(), entrySize);
}

// Returns encryptedContent.
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::addEntryValue(
    const vector<int>& accessControlProfileIds,
    string nameSpace, string name,
    vector<uint8_t> content) {
    vector<uint8_t> eicEncryptedContent;
    eicEncryptedContent.resize(content.size() + 28);
    if (!eicProvisioningAddEntryValue(&ctx_, accessControlProfileIds.data(),
                                   accessControlProfileIds.size(), nameSpace.c_str(),
                                   name.c_str(), content.data(), content.size(),
                                   eicEncryptedContent.data())) {
      return {};
    }
    return eicEncryptedContent;
}

// Returns signatureToBeSignedWithProofOfProvisioning (EIC_ECDSA_P256_SIGNATURE_SIZE bytes).
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::finishAddingEntries(bool testCredential) {
    vector<uint8_t> signatureToBeSignedWithProofOfProvisioning(EIC_ECDSA_P256_SIGNATURE_SIZE);
    if (!eicProvisioningFinishAddingEntries(&ctx_,
                                         testCredential,
                                         signatureToBeSignedWithProofOfProvisioning.data())) {
        return {};
    }
    return signatureToBeSignedWithProofOfProvisioning;
}

// Returns encryptedCredentialKeys (80 bytes).
optional<vector<uint8_t>> FakeSecureHardwareProvisioningProxy::finishGetCredentialData(bool testCredential,
                                                                                       string docType) {
    vector<uint8_t> encryptedCredentialKeys(80);
    if (!eicProvisioningFinishGetCredentialData(&ctx_,
                                             testCredential,
                                             docType.c_str(),
                                             encryptedCredentialKeys.data())) {
        return {};
    }
    return encryptedCredentialKeys;
}

// ----------------------------------------------------------------------

FakeSecureHardwarePresentationProxy::FakeSecureHardwarePresentationProxy() {
}

FakeSecureHardwarePresentationProxy::~FakeSecureHardwarePresentationProxy() {
}

bool FakeSecureHardwarePresentationProxy::initialize(bool testCredential,
                                                     string docType,
                                                     vector<uint8_t> encryptedCredentialKeys) {
  LOG(INFO) << "FakeSecureHardwarePresentationProxy created, sizeof(EicPresentation): "
            << sizeof(EicProvisioning);
  return eicPresentationInit(&ctx_,
                             testCredential,
                             docType.c_str(),
                             encryptedCredentialKeys.data());
}

// Returns publicKeyCert (1st component) and signingKeyBlob (2nd component)
optional<pair<vector<uint8_t>, vector<uint8_t>>>
FakeSecureHardwarePresentationProxy::generateSigningKeyPair(string docType, time_t now) {
    uint8_t publicKeyCert[512];
    size_t publicKeyCertSize = sizeof(publicKeyCert);
    vector<uint8_t> signingKeyBlob(60);

    if (!eicPresentationGenerateSigningKeyPair(&ctx_,
                                               docType.c_str(),
                                               now,
                                               publicKeyCert,
                                               &publicKeyCertSize,
                                               signingKeyBlob.data())) {
        return {};
    }

    vector<uint8_t> cert;
    cert.resize(publicKeyCertSize);
    memcpy(cert.data(), publicKeyCert, publicKeyCertSize);

    return std::make_pair(cert, signingKeyBlob);
}

// Returns private (1st component) and public key (2nd component).
optional<pair<vector<uint8_t>, vector<uint8_t>>>
FakeSecureHardwarePresentationProxy::createEphemeralKeyPair() {
    vector<uint8_t> priv(EIC_P256_PRIV_KEY_SIZE);
    if (!eicPresentationCreateEphemeralKeyPair(&ctx_, priv.data())) {
        return {};
    }
    vector<uint8_t> pub(EIC_P256_PUB_KEY_SIZE);
    memcpy(pub.data(), ctx_.ephemeralPublicKey, EIC_P256_PUB_KEY_SIZE);
    return make_pair(priv, pub);
}

bool FakeSecureHardwarePresentationProxy::shutdown() {
  LOG(INFO) << "FakeSecureHardwarePresentationProxy shutdown";
  return true;
}

}  // namespace android::hardware::identity

// ----------------------------------------------------------------------
// Implementation of eicOps and eic platform functions follow.
//

void* eicMemCpy(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

size_t eicStrLen(const char* s) {
    return strlen(s);
}

int eicMemCmp(const void* s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n);
}

void eicOpsSha256Init(EicSha256Ctx* ctx) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Init(realCtx);
}

void eicOpsSha256Update(EicSha256Ctx* ctx, const uint8_t* data, size_t len) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Update(realCtx, data, len);
}

void eicOpsSha256Final(EicSha256Ctx* ctx, uint8_t digest[EIC_SHA256_DIGEST_SIZE]) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Final(digest, realCtx);
}

bool eicOpsRandom(uint8_t* buf, size_t numBytes) {
    optional<vector<uint8_t>> bytes = ::android::hardware::identity::support::getRandom(numBytes);
    if (!bytes.has_value()) {
        return false;
    }
    memcpy(buf, bytes.value().data(), numBytes);
    return true;
}

bool eicOpsEncryptAes128Gcm(
        const uint8_t* key,    // Must be 16 bytes
        const uint8_t* nonce,  // Must be 12 bytes
        const uint8_t* data,   // May be NULL if size is 0
        size_t dataSize,
        const uint8_t* additionalAuthenticationData,  // May be NULL if size is 0
        size_t additionalAuthenticationDataSize, uint8_t* encryptedData) {
    vector<uint8_t> cppKey;
    cppKey.resize(16);
    memcpy(cppKey.data(), key, 16);

    vector<uint8_t> cppData;
    cppData.resize(dataSize);
    if (dataSize > 0) {
        memcpy(cppData.data(), data, dataSize);
    }

    vector<uint8_t> cppAAD;
    cppAAD.resize(additionalAuthenticationDataSize);
    if (additionalAuthenticationDataSize > 0) {
        memcpy(cppAAD.data(), additionalAuthenticationData, additionalAuthenticationDataSize);
    }

    vector<uint8_t> cppNonce;
    cppNonce.resize(12);
    memcpy(cppNonce.data(), nonce, 12);

    optional<vector<uint8_t>> cppEncryptedData =
        android::hardware::identity::support::encryptAes128Gcm(cppKey, cppNonce, cppData,
                                                               cppAAD);
    if (!cppEncryptedData.has_value()) {
        return false;
    }

    memcpy(encryptedData, cppEncryptedData.value().data(), cppEncryptedData.value().size());
    return true;
}

// Decrypts |encryptedData| using |key| and |additionalAuthenticatedData|,
// returns resulting plaintext in |data| must be of size |encryptedDataSize| - 28.
//
// The format of |encryptedData| must be as specified in the
// encryptAes128Gcm() function.
bool eicOpsDecryptAes128Gcm(const uint8_t* key,  // Must be 16 bytes
                            const uint8_t* encryptedData, size_t encryptedDataSize,
                            const uint8_t* additionalAuthenticationData,
                            size_t additionalAuthenticationDataSize,
                            uint8_t* data) {
    vector<uint8_t> keyVec;
    keyVec.resize(16);
    memcpy(keyVec.data(), key, 16);

    vector<uint8_t> encryptedDataVec;
    encryptedDataVec.resize(encryptedDataSize);
    if (encryptedDataSize > 0) {
        memcpy(encryptedDataVec.data(), encryptedData, encryptedDataSize);
    }

    vector<uint8_t> aadVec;
    aadVec.resize(additionalAuthenticationDataSize);
    if (additionalAuthenticationDataSize > 0) {
        memcpy(aadVec.data(), additionalAuthenticationData, additionalAuthenticationDataSize);
    }

    optional<vector<uint8_t>> decryptedDataVec =
        android::hardware::identity::support::decryptAes128Gcm(keyVec, encryptedDataVec, aadVec);
    if (!decryptedDataVec.has_value()) {
        eicDebug("Error decrypting data");
        return false;
    }
    if (decryptedDataVec.value().size() != encryptedDataSize - 28) {
        eicDebug("Decrypted data is size %zd, expected %zd", decryptedDataVec.value().size(),
                 encryptedDataSize - 28);
        return false;
    }

    memcpy(data, decryptedDataVec.value().data(), decryptedDataVec.value().size());
    return true;
}

bool eicOpsCreateEcKey(uint8_t privateKey[EIC_P256_PRIV_KEY_SIZE],
                       uint8_t publicKey[EIC_P256_PUB_KEY_SIZE]) {
  optional<vector<uint8_t>> keyPair = android::hardware::identity::support::createEcKeyPair();
  if (!keyPair) {
    eicDebug("Error creating EC keypair");
    return false;
  }
  optional<vector<uint8_t>> privKey = android::hardware::identity::support::ecKeyPairGetPrivateKey(keyPair.value());
  if (!privKey) {
    eicDebug("Error extracting private key");
    return false;
  }
  if (privKey.value().size() != EIC_P256_PRIV_KEY_SIZE) {
    eicDebug("Private key is not %zd bytes long as expected", (size_t) EIC_P256_PRIV_KEY_SIZE);
    return false;
  }

  optional<vector<uint8_t>> pubKey = android::hardware::identity::support::ecKeyPairGetPublicKey(keyPair.value());
  if (!pubKey) {
    eicDebug("Error extracting public key");
    return false;
  }
  // ecKeyPairGetPublicKey() returns 0x04 | x | y, we don't want the leading 0x04.
  if (pubKey.value().size() != EIC_P256_PUB_KEY_SIZE + 1) {
    eicDebug("Private key is not %zd bytes long as expected", (size_t) EIC_P256_PRIV_KEY_SIZE);
    return false;
  }

  memcpy(privateKey, privKey.value().data(), EIC_P256_PRIV_KEY_SIZE);
  memcpy(publicKey, pubKey.value().data() + 1, EIC_P256_PUB_KEY_SIZE);

  return true;
}

static const uint8_t eicAttestationPublicKey[EIC_P256_PUB_KEY_SIZE] = {
  0x3a, 0x16, 0x66, 0x7e, 0xc6, 0x70, 0x5c, 0x81, 0x36, 0x9c, 0x53, 0xe2, 0xb8, 0x39, 0x15, 0x2b,
  0x71, 0xad, 0x08, 0xd0, 0xa0, 0x63, 0x05, 0x12, 0xe6, 0xb9, 0x1b, 0x96, 0x58, 0xbe, 0x91, 0xbf,
  0x9c, 0xc2, 0x53, 0x02, 0x46, 0x0c, 0xd9, 0xd3, 0x53, 0x36, 0xf4, 0xf9, 0x9b, 0x42, 0x5d, 0xa1,
  0x91, 0x91, 0xcb, 0x23, 0x23, 0xb8, 0xb5, 0x3b, 0xe9, 0x45, 0xd9, 0xd8, 0x06, 0x29, 0xac, 0x47
};

static const uint8_t eicAttestationPrivateKey[EIC_P256_PRIV_KEY_SIZE] = {
  0x81, 0xdb, 0x31, 0x67, 0x52, 0xb9, 0xab, 0xc5, 0xe8, 0xc3, 0x1c, 0xb9, 0x92, 0xb3, 0x06, 0x27,
  0x33, 0x81, 0xf7, 0xc0, 0xdb, 0x98, 0x93, 0x37, 0x4f, 0xae, 0x08, 0xc0, 0x5e, 0x3f, 0x7e, 0x4a
};

const char* eicOpsGetIssuerName(void) {
  return "AOSP libEmbeddedIC (Google)";
}

const uint8_t* eicOpsGetAttestationPublicKey(void) {
  return eicAttestationPublicKey;
}

bool eicOpsAttestToEcKey(const uint8_t publicKey[EIC_P256_PUB_KEY_SIZE],
                         const uint8_t* challenge,
                         size_t challengeSize,
                         const uint8_t* applicationId,
                         size_t applicationIdSize,
                         uint8_t* cert,
                         size_t* certSize) {  // inout
  vector<uint8_t> signingKeyVec(EIC_P256_PRIV_KEY_SIZE);
  memcpy(signingKeyVec.data(), eicAttestationPrivateKey, EIC_P256_PRIV_KEY_SIZE);


  vector<uint8_t> pubKeyVec(EIC_P256_PUB_KEY_SIZE + 1);
  pubKeyVec[0] = 0x04;
  memcpy(pubKeyVec.data() + 1, publicKey, EIC_P256_PUB_KEY_SIZE);

  // TODO: create attestation extension with challenge, applicationId, others
  //
  // The android.security.identity.cts.AttestationTest#attestationTest won't
  // pass until this is done.

  const int secondsInOneYear = 365 * 24 * 60 * 60;
  time_t validityNotBefore = time(nullptr); // now
  time_t validityNotAfter = validityNotBefore + secondsInOneYear; // A year from now.

  optional<vector<uint8_t>> certVec =
      android::hardware::identity::support::ecPublicKeyGenerateCertificate(
          pubKeyVec,
          signingKeyVec,
          "1",
          eicOpsGetIssuerName(),
          "Android Identity Credential Credential Key",
          validityNotBefore,
          validityNotAfter);
  if (!certVec) {
    eicDebug("Error generating attestation");
    return false;
  }

  if (*certSize < certVec.value().size()) {
    eicDebug("Buffer for certificate is only %zd bytes long, need %zd bytes",
             *certSize, certVec.value().size());
    return false;
  }
  memcpy(cert, certVec.value().data(), certVec.value().size());
  *certSize = certVec.value().size();

  return true;
}

bool eicOpsSignEcKey(const uint8_t publicKey[EIC_P256_PUB_KEY_SIZE],
                     const uint8_t signingKey[EIC_P256_PRIV_KEY_SIZE],
                     unsigned int serial,
                     const char* issuerName,
                     const char* subjectName,
                     time_t validityNotBefore,
                     time_t validityNotAfter,
                     uint8_t* cert,
                     size_t* certSize) {  // inout
  vector<uint8_t> signingKeyVec(EIC_P256_PRIV_KEY_SIZE);
  memcpy(signingKeyVec.data(), signingKey, EIC_P256_PRIV_KEY_SIZE);

  vector<uint8_t> pubKeyVec(EIC_P256_PUB_KEY_SIZE + 1);
  pubKeyVec[0] = 0x04;
  memcpy(pubKeyVec.data() + 1, publicKey, EIC_P256_PUB_KEY_SIZE);

  std::string serialDecimal = android::base::StringPrintf("%d", serial);

  optional<vector<uint8_t>> certVec =
      android::hardware::identity::support::ecPublicKeyGenerateCertificate(pubKeyVec,
                                                                           signingKeyVec,
                                                                           serialDecimal,
                                                                           issuerName,
                                                                           subjectName,
                                                                           validityNotBefore,
                                                                           validityNotAfter);
  if (!certVec) {
    eicDebug("Error generating certificate");
    return false;
  }

  if (*certSize < certVec.value().size()) {
    eicDebug("Buffer for certificate is only %zd bytes long, need %zd bytes",
             *certSize, certVec.value().size());
    return false;
  }
  memcpy(cert, certVec.value().data(), certVec.value().size());
  *certSize = certVec.value().size();

  return true;
}

bool eicOpsEcDsa(const uint8_t privateKey[EIC_P256_PRIV_KEY_SIZE],
                 const uint8_t digestOfData[EIC_SHA256_DIGEST_SIZE],
                 uint8_t signature[EIC_ECDSA_P256_SIGNATURE_SIZE]) {

    vector<uint8_t> privKeyVec(EIC_P256_PRIV_KEY_SIZE);
    memcpy(privKeyVec.data(), privateKey, EIC_P256_PRIV_KEY_SIZE);

    vector<uint8_t> digestVec(EIC_SHA256_DIGEST_SIZE);
    memcpy(digestVec.data(), digestOfData, EIC_SHA256_DIGEST_SIZE);

    optional<vector<uint8_t>> derSignature =
        android::hardware::identity::support::signEcDsaDigest(privKeyVec, digestVec);
    if (!derSignature) {
        eicDebug("Error signing data");
        return false;
    }

    ECDSA_SIG* sig;
    const unsigned char* p = derSignature.value().data();
    sig = d2i_ECDSA_SIG(nullptr, &p, derSignature.value().size());
    if (sig == nullptr) {
        eicDebug("Error decoding DER signature");
        return false;
    }

    if (BN_bn2binpad(sig->r, signature, 32) != 32) {
        eicDebug("Error encoding r");
        return false;
    }
    if (BN_bn2binpad(sig->s, signature + 32, 32) != 32) {
        eicDebug("Error encoding s");
        return false;
    }

    return true;
}

static const uint8_t hbkTest[16] = {0};
static const uint8_t hbkReal[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const uint8_t* eicOpsGetHardwareBoundKey(bool testCredential) {
  if (testCredential) {
    return hbkTest;
  }
  return hbkReal;
}

#ifdef EIC_DEBUG

void eicPrint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void eicHexdump(const char* message, const uint8_t* data, size_t dataSize) {
    vector<uint8_t> dataVec(dataSize);
    memcpy(dataVec.data(), data, dataSize);
    android::hardware::identity::support::hexdump(message, dataVec);
}

void eicCborPrettyPrint(const uint8_t* cborData, size_t cborDataSize, size_t maxBStrSize) {
    vector<uint8_t> cborDataVec(cborDataSize);
    memcpy(cborDataVec.data(), cborData, cborDataSize);
    string str = android::hardware::identity::support::cborPrettyPrint(cborDataVec, maxBStrSize, {});
    fprintf(stderr, "%s\n", str.c_str());
}

#endif  // EIC_DEBUG
