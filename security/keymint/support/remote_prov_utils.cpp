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

#include <iomanip>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include "aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h"

#include <aidl/android/hardware/security/keymint/RpcHardwareInfo.h>
#include <android-base/macros.h>
#include <android-base/properties.h>
#include <authenticated_request.h>
#include <cppbor.h>
#include <json/json.h>
#include <keymaster/km_openssl/ecdsa_operation.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <openssl/base64.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <remote_prov/remote_prov_utils.h>

#include "include/remote_prov/remote_prov_utils_common.h"

namespace aidl::android::hardware::security::keymint::remote_prov {

constexpr uint32_t kNumTeeDeviceInfoEntries = 14;

ErrMsgOr<bytevec> constructCoseKey(int32_t supportedEekCurve, const bytevec& eekId,
                                   const bytevec& pubKey) {
    CoseKeyType keyType;
    CoseKeyAlgorithm algorithm;
    CoseKeyCurve curve;
    bytevec pubX;
    bytevec pubY;
    switch (supportedEekCurve) {
    case RpcHardwareInfo::CURVE_25519:
        keyType = OCTET_KEY_PAIR;
        algorithm = (eekId.empty()) ? EDDSA : ECDH_ES_HKDF_256;
        curve = (eekId.empty()) ? ED25519 : cppcose::X25519;
        pubX = pubKey;
        break;
    case RpcHardwareInfo::CURVE_P256: {
        keyType = EC2;
        algorithm = (eekId.empty()) ? ES256 : ECDH_ES_HKDF_256;
        curve = P256;
        auto affineCoordinates = getAffineCoordinates(pubKey);
        if (!affineCoordinates) return affineCoordinates.moveMessage();
        std::tie(pubX, pubY) = affineCoordinates.moveValue();
    } break;
    default:
        return "Unknown EEK Curve.";
    }
    cppbor::Map coseKey = cppbor::Map()
                              .add(CoseKey::KEY_TYPE, keyType)
                              .add(CoseKey::ALGORITHM, algorithm)
                              .add(CoseKey::CURVE, curve)
                              .add(CoseKey::PUBKEY_X, pubX);

    if (!pubY.empty()) coseKey.add(CoseKey::PUBKEY_Y, pubY);
    if (!eekId.empty()) coseKey.add(CoseKey::KEY_ID, eekId);

    return coseKey.canonicalize().encode();
}

bytevec kTestMacKey(32 /* count */, 0 /* byte value */);

bytevec randomBytes(size_t numBytes) {
    bytevec retval(numBytes);
    RAND_bytes(retval.data(), numBytes);
    return retval;
}

ErrMsgOr<cppbor::Array> constructCoseSign1(int32_t supportedEekCurve, const bytevec& key,
                                           const bytevec& payload, const bytevec& aad) {
    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        return constructECDSACoseSign1(key, {} /* protectedParams */, payload, aad);
    } else {
        return cppcose::constructCoseSign1(key, payload, aad);
    }
}

ErrMsgOr<EekChain> generateEekChain(int32_t supportedEekCurve, size_t length,
                                    const bytevec& eekId) {
    if (length < 2) {
        return "EEK chain must contain at least 2 certs.";
    }

    auto eekChain = cppbor::Array();

    bytevec prev_priv_key;
    for (size_t i = 0; i < length - 1; ++i) {
        auto keyPair = generateKeyPair(supportedEekCurve, false);
        if (!keyPair) return keyPair.moveMessage();
        auto [pub_key, priv_key] = keyPair.moveValue();

        // The first signing key is self-signed.
        if (prev_priv_key.empty()) prev_priv_key = priv_key;

        auto coseKey = constructCoseKey(supportedEekCurve, {}, pub_key);
        if (!coseKey) return coseKey.moveMessage();

        auto coseSign1 =
            constructCoseSign1(supportedEekCurve, prev_priv_key, coseKey.moveValue(), {} /* AAD */);
        if (!coseSign1) return coseSign1.moveMessage();
        eekChain.add(coseSign1.moveValue());

        prev_priv_key = priv_key;
    }
    auto keyPair = generateKeyPair(supportedEekCurve, true);
    if (!keyPair) return keyPair.moveMessage();
    auto [pub_key, priv_key] = keyPair.moveValue();

    auto coseKey = constructCoseKey(supportedEekCurve, eekId, pub_key);
    if (!coseKey) return coseKey.moveMessage();

    auto coseSign1 =
        constructCoseSign1(supportedEekCurve, prev_priv_key, coseKey.moveValue(), {} /* AAD */);
    if (!coseSign1) return coseSign1.moveMessage();
    eekChain.add(coseSign1.moveValue());

    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        // convert ec public key to x and y co-ordinates.
        auto affineCoordinates = getAffineCoordinates(pub_key);
        if (!affineCoordinates) return affineCoordinates.moveMessage();
        auto [pubX, pubY] = affineCoordinates.moveValue();
        pub_key.clear();
        pub_key.insert(pub_key.begin(), pubX.begin(), pubX.end());
        pub_key.insert(pub_key.end(), pubY.begin(), pubY.end());
    }

    return EekChain{eekChain.encode(), pub_key, priv_key};
}

bytevec getProdEekChain(int32_t supportedEekCurve) {
    cppbor::Array chain;
    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        chain.add(cppbor::EncodedItem(bytevec(std::begin(kCoseEncodedEcdsa256RootCert),
                                              std::end(kCoseEncodedEcdsa256RootCert))));
        chain.add(cppbor::EncodedItem(bytevec(std::begin(kCoseEncodedEcdsa256GeekCert),
                                              std::end(kCoseEncodedEcdsa256GeekCert))));
    } else {
        chain.add(cppbor::EncodedItem(
            bytevec(std::begin(kCoseEncodedRootCert), std::end(kCoseEncodedRootCert))));
        chain.add(cppbor::EncodedItem(
            bytevec(std::begin(kCoseEncodedGeekCert), std::end(kCoseEncodedGeekCert))));
    }
    return chain.encode();
}

JsonOutput jsonEncodeCsrWithBuild(const std::string& instance_name, const cppbor::Array& csr,
                                  const std::string& serialno_prop) {
    const std::string kFingerprintProp = "ro.build.fingerprint";

    if (!::android::base::WaitForPropertyCreation(kFingerprintProp)) {
        return JsonOutput::Error("Unable to read build fingerprint");
    }

    bytevec csrCbor = csr.encode();
    size_t base64Length;
    int rc = EVP_EncodedLength(&base64Length, csrCbor.size());
    if (!rc) {
        return JsonOutput::Error("Error getting base64 length. Size overflow?");
    }

    std::vector<char> base64(base64Length);
    rc = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(base64.data()), csrCbor.data(), csrCbor.size());
    ++rc;  // Account for NUL, which BoringSSL does not for some reason.
    if (rc != base64Length) {
        return JsonOutput::Error("Error writing base64. Expected " + std::to_string(base64Length) +
                                 " bytes to be written, but " + std::to_string(rc) +
                                 " bytes were actually written.");
    }

    Json::Value json(Json::objectValue);
    json["name"] = instance_name;
    json["build_fingerprint"] = ::android::base::GetProperty(kFingerprintProp, /*default=*/"");
    json["serialno"] = ::android::base::GetProperty(serialno_prop, /*default=*/"");
    json["csr"] = base64.data();  // Boring writes a NUL-terminated c-string

    Json::StreamWriterBuilder factory;
    factory["indentation"] = "";  // disable pretty formatting
    return JsonOutput::Ok(Json::writeString(factory, json));
}

std::string checkMapEntry(bool isFactory, const cppbor::Map& devInfo, cppbor::MajorType majorType,
                          const std::string& entryName) {
    const std::unique_ptr<cppbor::Item>& val = devInfo.get(entryName);
    if (!val) {
        return entryName + " is missing.\n";
    }
    if (val->type() != majorType) {
        return entryName + " has the wrong type.\n";
    }
    if (isFactory) {
        return "";
    }
    switch (majorType) {
        case cppbor::TSTR:
            if (val->asTstr()->value().size() <= 0) {
                return entryName + " is present but the value is empty.\n";
            }
            break;
        case cppbor::BSTR:
            if (val->asBstr()->value().size() <= 0) {
                return entryName + " is present but the value is empty.\n";
            }
            break;
        default:
            break;
    }
    return "";
}

std::string checkMapEntry(bool isFactory, const cppbor::Map& devInfo, cppbor::MajorType majorType,
                          const std::string& entryName, const cppbor::Array& allowList) {
    std::string error = checkMapEntry(isFactory, devInfo, majorType, entryName);
    if (!error.empty()) {
        return error;
    }

    if (isFactory) {
        return "";
    }

    const std::unique_ptr<cppbor::Item>& val = devInfo.get(entryName);
    for (auto i = allowList.begin(); i != allowList.end(); ++i) {
        if (**i == *val) {
            return "";
        }
    }
    return entryName + " has an invalid value.\n";
}

std::string checkMapPatchLevelEntry(bool isFactory, const cppbor::Map& devInfo,
                                    const std::string& entryName) {
    std::string error = checkMapEntry(isFactory, devInfo, cppbor::UINT, entryName);
    if (!error.empty()) {
        return error;
    }

    if (isFactory) {
        return "";
    }

    const std::unique_ptr<cppbor::Item>& val = devInfo.get(entryName);
    std::string dateString = std::to_string(val->asUint()->unsignedValue());
    if (dateString.size() == 6) {
        dateString += "01";
    }
    if (dateString.size() != 8) {
        return entryName + " should in the format YYYYMMDD or YYYYMM\n";
    }

    std::tm t;
    std::istringstream ss(dateString);
    ss >> std::get_time(&t, "%Y%m%d");
    if (!ss) {
        return entryName + " should in the format YYYYMMDD or YYYYMM\n";
    }

    return "";
}

bool isTeeDeviceInfo(const cppbor::Map& devInfo) {
    return devInfo.get("security_level") && devInfo.get("security_level")->asTstr() &&
           devInfo.get("security_level")->asTstr()->value() == "tee";
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable,
        bool isFactory) {
    const cppbor::Array kValidVbStates = {"green", "yellow", "orange"};
    const cppbor::Array kValidBootloaderStates = {"locked", "unlocked"};
    const cppbor::Array kValidSecurityLevels = {"tee", "strongbox"};
    const cppbor::Array kValidAttIdStates = {"locked", "open"};
    const cppbor::Array kValidFused = {0, 1};
    constexpr std::array<std::string_view, kNumTeeDeviceInfoEntries> kDeviceInfoKeys = {
            "brand",
            "manufacturer",
            "product",
            "model",
            "device",
            "vb_state",
            "bootloader_state",
            "vbmeta_digest",
            "os_version",
            "system_patch_level",
            "boot_patch_level",
            "vendor_patch_level",
            "security_level",
            "fused"};

    struct AttestationIdEntry {
        const char* id;
        bool alwaysValidate;
    };
    constexpr AttestationIdEntry kAttestationIdEntrySet[] = {{"brand", false},
                                                             {"manufacturer", true},
                                                             {"product", false},
                                                             {"model", false},
                                                             {"device", false}};

    auto [parsedVerifiedDeviceInfo, ignore1, errMsg] = cppbor::parse(deviceInfoBytes);
    if (!parsedVerifiedDeviceInfo) {
        return errMsg;
    }

    std::unique_ptr<cppbor::Map> parsed(parsedVerifiedDeviceInfo.release()->asMap());
    if (!parsed) {
        return "DeviceInfo must be a CBOR map.";
    }

    if (parsed->clone()->asMap()->canonicalize().encode() != deviceInfoBytes) {
        return "DeviceInfo ordering is non-canonical.";
    }

    RpcHardwareInfo info;
    provisionable->getHardwareInfo(&info);
    if (info.versionNumber < 3) {
        const std::unique_ptr<cppbor::Item>& version = parsed->get("version");
        if (!version) {
            return "Device info is missing version";
        }
        if (!version->asUint()) {
            return "version must be an unsigned integer";
        }
        if (version->asUint()->value() != info.versionNumber) {
            return "DeviceInfo version (" + std::to_string(version->asUint()->value()) +
                   ") does not match the remotely provisioned component version (" +
                   std::to_string(info.versionNumber) + ").";
        }
    }
    // Bypasses the device info validation since the device info in AVF is currently
    // empty. Check b/299256925 for more information.
    //
    // TODO(b/300911665): This check is temporary and will be replaced once the markers
    // on the DICE chain become available. We need to determine if the CSR is from the
    // RKP VM using the markers on the DICE chain.
    if (info.uniqueId == "AVF Remote Provisioning 1") {
        return std::move(parsed);
    }

    std::string error;
    std::string tmp;
    std::set<std::string_view> previousKeys;
    switch (info.versionNumber) {
        case 3:
            if (isTeeDeviceInfo(*parsed) && parsed->size() != kNumTeeDeviceInfoEntries) {
                error += fmt::format(
                        "Err: Incorrect number of device info entries. Expected {} but got "
                        "{}\n",
                        kNumTeeDeviceInfoEntries, parsed->size());
            }
            // TEE IRPC instances require all entries to be present in DeviceInfo. Non-TEE instances
            // may omit `os_version`
            if (!isTeeDeviceInfo(*parsed) && (parsed->size() != kNumTeeDeviceInfoEntries &&
                                              parsed->size() != kNumTeeDeviceInfoEntries - 1)) {
                error += fmt::format(
                        "Err: Incorrect number of device info entries. Expected {} or {} but got "
                        "{}\n",
                        kNumTeeDeviceInfoEntries - 1, kNumTeeDeviceInfoEntries, parsed->size());
            }
            for (auto& [key, _] : *parsed) {
                const std::string& keyValue = key->asTstr()->value();
                if (!previousKeys.insert(keyValue).second) {
                    error += "Err: Duplicate device info entry: <" + keyValue + ">,\n";
                }
                if (std::find(kDeviceInfoKeys.begin(), kDeviceInfoKeys.end(), keyValue) ==
                    kDeviceInfoKeys.end()) {
                    error += "Err: Unrecognized key entry: <" + key->asTstr()->value() + ">,\n";
                }
            }
            // Checks that only apply to v3.
            error += checkMapPatchLevelEntry(isFactory, *parsed, "system_patch_level");
            error += checkMapPatchLevelEntry(isFactory, *parsed, "boot_patch_level");
            error += checkMapPatchLevelEntry(isFactory, *parsed, "vendor_patch_level");
            FALLTHROUGH_INTENDED;
        case 2:
            for (const auto& entry : kAttestationIdEntrySet) {
                tmp = checkMapEntry(isFactory && !entry.alwaysValidate, *parsed, cppbor::TSTR,
                                    entry.id);
            }
            if (!tmp.empty()) {
                error += tmp +
                         "Attestation IDs are missing or malprovisioned. If this test is being\n"
                         "run against an early proto or EVT build, this error is probably WAI\n"
                         "and indicates that Device IDs were not provisioned in the factory. If\n"
                         "this error is returned on a DVT or later build revision, then\n"
                         "something is likely wrong with the factory provisioning process.";
            }
            // TODO: Refactor the KeyMint code that validates these fields and include it here.
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "vb_state", kValidVbStates);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "bootloader_state",
                                   kValidBootloaderStates);
            error += checkMapEntry(isFactory, *parsed, cppbor::BSTR, "vbmeta_digest");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "system_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "boot_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "vendor_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "fused", kValidFused);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "security_level",
                                   kValidSecurityLevels);
            if (isTeeDeviceInfo(*parsed)) {
                error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "os_version");
            }
            break;
        case 1:
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "security_level",
                                   kValidSecurityLevels);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "att_id_state",
                                   kValidAttIdStates);
            break;
        default:
            return "Unrecognized version: " + std::to_string(info.versionNumber);
    }

    if (!error.empty()) {
        return error;
    }

    return std::move(parsed);
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateFactoryDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable) {
    return parseAndValidateDeviceInfo(deviceInfoBytes, provisionable, /*isFactory=*/true);
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateProductionDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable) {
    return parseAndValidateDeviceInfo(deviceInfoBytes, provisionable, /*isFactory=*/false);
}

ErrMsgOr<bytevec> getSessionKey(ErrMsgOr<std::pair<bytevec, bytevec>>& senderPubkey,
                                const EekChain& eekChain, int32_t supportedEekCurve) {
    if (supportedEekCurve == RpcHardwareInfo::CURVE_25519 ||
        supportedEekCurve == RpcHardwareInfo::CURVE_NONE) {
        return x25519_HKDF_DeriveKey(eekChain.last_pubkey, eekChain.last_privkey,
                                     senderPubkey->first, false /* senderIsA */);
    } else {
        return ECDH_HKDF_DeriveKey(eekChain.last_pubkey, eekChain.last_privkey, senderPubkey->first,
                                   false /* senderIsA */);
    }
}

ErrMsgOr<std::vector<DiceCertChainEntry>> verifyProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::string& instanceName,
        const std::vector<uint8_t>& challenge, bool isFactory, bool allowAnyMode = false) {
    auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData.protectedData);
    if (!parsedProtectedData) {
        return protDataErrMsg;
    }
    if (!parsedProtectedData->asArray()) {
        return "Protected data is not a CBOR array.";
    }
    if (parsedProtectedData->asArray()->size() != kCoseEncryptEntryCount) {
        return "The protected data COSE_encrypt structure must have " +
               std::to_string(kCoseEncryptEntryCount) + " entries, but it only has " +
               std::to_string(parsedProtectedData->asArray()->size());
    }

    auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
    if (!senderPubkey) {
        return senderPubkey.message();
    }
    if (senderPubkey->second != eekId) {
        return "The COSE_encrypt recipient does not match the expected EEK identifier";
    }

    auto sessionKey = getSessionKey(senderPubkey, eekChain, supportedEekCurve);
    if (!sessionKey) {
        return sessionKey.message();
    }

    auto protectedDataPayload =
            decryptCoseEncrypt(*sessionKey, parsedProtectedData.get(), bytevec{} /* aad */);
    if (!protectedDataPayload) {
        return protectedDataPayload.message();
    }

    auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(*protectedDataPayload);
    if (!parsedPayload) {
        return "Failed to parse payload: " + payloadErrMsg;
    }
    if (!parsedPayload->asArray()) {
        return "The protected data payload must be an Array.";
    }
    if (parsedPayload->asArray()->size() != 3U && parsedPayload->asArray()->size() != 2U) {
        return "The protected data payload must contain SignedMAC and BCC. It may optionally "
               "contain AdditionalDKSignatures. However, the parsed payload has " +
               std::to_string(parsedPayload->asArray()->size()) + " entries.";
    }

    auto& signedMac = parsedPayload->asArray()->get(0);
    auto& diceCertChain = parsedPayload->asArray()->get(1);
    if (!signedMac->asArray()) {
        return "The SignedMAC in the protected data payload is not an Array.";
    }
    if (!diceCertChain->asArray()) {
        return "The  in the protected data payload is not an Array.";
    }

    // BCC is [ pubkey, + BccEntry]
    auto encodedDiceCertChain = diceCertChain->asArray()->encode();
    auto result = validateDiceCertChain(encodedDiceCertChain, hwtrust::DiceChain::Kind::kVsr13,
                                        allowAnyMode, instanceName);
    if (!result) {
        return result.message() + "\n" + prettyPrint(diceCertChain.get());
    }

    auto [isProper, diceCertChainEntries] = *result;

    auto deviceInfoResult =
            parseAndValidateDeviceInfo(deviceInfo.deviceInfo, provisionable, isFactory);
    if (!deviceInfoResult) {
        return deviceInfoResult.message();
    }
    std::unique_ptr<cppbor::Map> deviceInfoMap = deviceInfoResult.moveValue();
    auto& leafSigningKey = diceCertChainEntries.back().pubKey;
    auto macKey = verifyAndParseCoseSign1(signedMac->asArray(), leafSigningKey,
                                          cppbor::Array()  // SignedMacAad
                                                  .add(challenge)
                                                  .add(std::move(deviceInfoMap))
                                                  .add(keysToSignMac)
                                                  .encode());
    if (!macKey) {
        return macKey.message();
    }

    auto coseMac0 = cppbor::Array()
                            .add(cppbor::Map()  // protected
                                         .add(ALGORITHM, HMAC_256)
                                         .canonicalize()
                                         .encode())
                            .add(cppbor::Map())        // unprotected
                            .add(keysToSign.encode())  // payload (keysToSign)
                            .add(keysToSignMac);       // tag

    auto macPayload = verifyAndParseCoseMac0(&coseMac0, *macKey);
    if (!macPayload) {
        return macPayload.message();
    }

    return diceCertChainEntries;
}

ErrMsgOr<std::vector<DiceCertChainEntry>> verifyFactoryProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::string& instanceName,
        const std::vector<uint8_t>& challenge) {
    return verifyProtectedData(deviceInfo, keysToSign, keysToSignMac, protectedData, eekChain,
                               eekId, supportedEekCurve, provisionable, instanceName, challenge,
                               /*isFactory=*/true);
}

ErrMsgOr<std::vector<DiceCertChainEntry>> verifyProductionProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::string& instanceName,
        const std::vector<uint8_t>& challenge, bool allowAnyMode) {
    return verifyProtectedData(deviceInfo, keysToSign, keysToSignMac, protectedData, eekChain,
                               eekId, supportedEekCurve, provisionable, instanceName, challenge,
                               /*isFactory=*/false, allowAnyMode);
}

ErrMsgOr<std::unique_ptr<cppbor::Array>> parseAndValidateCsrPayload(
        const cppbor::Array& keysToSign, const std::vector<uint8_t>& csrPayload,
        IRemotelyProvisionedComponent* provisionable, bool isFactory) {
    auto [parsedCsrPayload, _, errMsg] = cppbor::parse(csrPayload);
    if (!parsedCsrPayload) {
        return errMsg;
    }

    std::unique_ptr<cppbor::Array> parsed(parsedCsrPayload.release()->asArray());
    if (!parsed) {
        return "CSR payload is not a CBOR array.";
    }

    if (parsed->size() != 4U) {
        return "CSR payload must contain version, certificate type, device info, keys. "
               "However, the parsed CSR payload has " +
               std::to_string(parsed->size()) + " entries.";
    }

    auto signedVersion = parsed->get(0)->asUint();
    auto signedCertificateType = parsed->get(1)->asTstr();
    auto signedDeviceInfo = parsed->get(2)->asMap();
    auto signedKeys = parsed->get(3)->asArray();

    if (!signedVersion || signedVersion->value() != 3U) {
        return "CSR payload version must be an unsigned integer and must be equal to 3.";
    }
    if (!signedCertificateType) {
        // Certificate type is allowed to be extendend by vendor, i.e. we can't
        // enforce its value.
        return "Certificate type must be a Tstr.";
    }
    if (!signedDeviceInfo) {
        return "Device info must be an Map.";
    }
    if (!signedKeys) {
        return "Keys must be an Array.";
    }

    auto result = parseAndValidateDeviceInfo(signedDeviceInfo->encode(), provisionable, isFactory);
    if (!result) {
        return result.message();
    }

    if (signedKeys->encode() != keysToSign.encode()) {
        return "Signed keys do not match.";
    }

    return std::move(parsed);
}

ErrMsgOr<std::unique_ptr<cppbor::Array>> verifyCsr(const cppbor::Array& keysToSign,
                                                   const std::vector<uint8_t>& csr,
                                                   IRemotelyProvisionedComponent* provisionable,
                                                   const std::string& instanceName,
                                                   const std::vector<uint8_t>& challenge,
                                                   bool isFactory, bool allowAnyMode,
                                                   bool allowDegenerate, bool requireUdsCerts) {
    RpcHardwareInfo info;
    provisionable->getHardwareInfo(&info);
    if (info.versionNumber != 3) {
        return "Remotely provisioned component version (" + std::to_string(info.versionNumber) +
               ") does not match expected version (3).";
    }

    auto authenticateRequest = AuthenticatedRequest(csr, challenge, instanceName, allowAnyMode,
                                                    allowDegenerate, requireUdsCerts);

    auto csrPayload = authenticateRequest.csrPayload();
    if (!csrPayload) {
        return csrPayload.message();
    }

    return parseAndValidateCsrPayload(keysToSign, *csrPayload, provisionable, isFactory);
}

ErrMsgOr<std::unique_ptr<cppbor::Array>> verifyFactoryCsr(
        const cppbor::Array& keysToSign, const std::vector<uint8_t>& csr,
        IRemotelyProvisionedComponent* provisionable, const std::string& instanceName,
        const std::vector<uint8_t>& challenge, bool allowDegenerate, bool requireUdsCerts) {
    return verifyCsr(keysToSign, csr, provisionable, instanceName, challenge, true /*isFactory*/,
                     false /*allowAnyMode*/, allowDegenerate, requireUdsCerts);
}

ErrMsgOr<std::unique_ptr<cppbor::Array>> verifyProductionCsr(
        const cppbor::Array& keysToSign, const std::vector<uint8_t>& csr,
        IRemotelyProvisionedComponent* provisionable, const std::string& instanceName,
        const std::vector<uint8_t>& challenge, bool allowAnyMode) {
    return verifyCsr(keysToSign, csr, provisionable, instanceName, challenge, false /*isFactory*/,
                     allowAnyMode, true /*allowDegenerate*/, false /*requireUdsCerts*/);
}

ErrMsgOr<bool> isCsrWithProperDiceChain(const std::vector<uint8_t>& csr,
                                        const std::vector<uint8_t>& challenge,
                                        const std::string& instanceName) {
    auto authenticateRequest =
            AuthenticatedRequest(csr, challenge, instanceName, false /*allowAnyMode*/,
                                 true /*allowDegenerate*/, false /*requireUdsCerts*/);

    return authenticateRequest.isProper();
}

ErrMsgOr<std::vector<uint8_t>> getUdsPubFromDiceCertChain(const std::vector<uint8_t>& request,
                                                          const std::vector<uint8_t>& challenge,
                                                          const std::string& instanceName) {
    auto authenticateRequest =
            AuthenticatedRequest(request, challenge, instanceName, false /*allowAnyMode*/,
                                 true /*allowDegenerate*/, false /*requireUdsCerts*/);

    auto udsPub = authenticateRequest.getUdsPubFromDiceChain();
    if (!udsPub) {
        return udsPub.message();
    }

    return *udsPub;
}

}  // namespace aidl::android::hardware::security::keymint::remote_prov
