/*
 * Copyright 2020 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/security/keymint/ErrorCode.h>

#include <keymint_support/attestation_record.h>
#include <keymint_support/authorization_set.h>
#include <keymint_support/openssl_utils.h>

#include <openssl/asn1t.h>

namespace aidl::android::hardware::security::keymint {

class AuthorizationSet;

/**
 * The OID for Android attestation records.  For the curious, it breaks down as follows:
 *
 * 1 = ISO
 * 3 = org
 * 6 = DoD (Huh? OIDs are weird.)
 * 1 = IANA
 * 4 = Private
 * 1 = Enterprises
 * 11129 = Google
 * 2 = Google security
 * 1 = certificate extension
 * 17 = Android attestation extension.
 */
static const char kAttestionRecordOid[] = "1.3.6.1.4.1.11129.2.1.17";

static const char kCrlDPOid[] = "2.5.29.31";  // Standard CRL Distribution Points extension.

struct stack_st_ASN1_TYPE_Delete {
    void operator()(stack_st_ASN1_TYPE* p) { sk_ASN1_TYPE_free(p); }
};

struct ASN1_STRING_Delete {
    void operator()(ASN1_STRING* p) { ASN1_STRING_free(p); }
};

struct ASN1_TYPE_Delete {
    void operator()(ASN1_TYPE* p) { ASN1_TYPE_free(p); }
};

#define ASN1_INTEGER_SET STACK_OF(ASN1_INTEGER)

typedef struct km_root_of_trust {
    ASN1_OCTET_STRING* verified_boot_key;
    ASN1_BOOLEAN device_locked;
    ASN1_ENUMERATED* verified_boot_state;
    ASN1_OCTET_STRING* verified_boot_hash;
} KM_ROOT_OF_TRUST;

ASN1_SEQUENCE(KM_ROOT_OF_TRUST) = {
        ASN1_SIMPLE(KM_ROOT_OF_TRUST, verified_boot_key, ASN1_OCTET_STRING),
        ASN1_SIMPLE(KM_ROOT_OF_TRUST, device_locked, ASN1_BOOLEAN),
        ASN1_SIMPLE(KM_ROOT_OF_TRUST, verified_boot_state, ASN1_ENUMERATED),
        ASN1_SIMPLE(KM_ROOT_OF_TRUST, verified_boot_hash, ASN1_OCTET_STRING),
} ASN1_SEQUENCE_END(KM_ROOT_OF_TRUST);
DECLARE_ASN1_FUNCTIONS(KM_ROOT_OF_TRUST);

// Fields ordered in tag order.
typedef struct km_auth_list {
    ASN1_INTEGER_SET* purpose;
    ASN1_INTEGER* algorithm;
    ASN1_INTEGER* key_size;
    ASN1_INTEGER_SET* digest;
    ASN1_INTEGER_SET* padding;
    ASN1_INTEGER* ec_curve;
    ASN1_INTEGER* rsa_public_exponent;
    ASN1_INTEGER_SET* mgf_digest;
    ASN1_NULL* rollback_resistance;
    ASN1_NULL* early_boot_only;
    ASN1_INTEGER* active_date_time;
    ASN1_INTEGER* origination_expire_date_time;
    ASN1_INTEGER* usage_expire_date_time;
    ASN1_INTEGER* usage_count_limit;
    ASN1_NULL* no_auth_required;
    ASN1_INTEGER* user_auth_type;
    ASN1_INTEGER* auth_timeout;
    ASN1_NULL* allow_while_on_body;
    ASN1_NULL* trusted_user_presence_required;
    ASN1_NULL* trusted_confirmation_required;
    ASN1_NULL* unlocked_device_required;
    ASN1_INTEGER* creation_date_time;
    ASN1_INTEGER* origin;
    KM_ROOT_OF_TRUST* root_of_trust;
    ASN1_INTEGER* os_version;
    ASN1_INTEGER* os_patchlevel;
    ASN1_OCTET_STRING* attestation_application_id;
    ASN1_OCTET_STRING* attestation_id_brand;
    ASN1_OCTET_STRING* attestation_id_device;
    ASN1_OCTET_STRING* attestation_id_product;
    ASN1_OCTET_STRING* attestation_id_serial;
    ASN1_OCTET_STRING* attestation_id_imei;
    ASN1_OCTET_STRING* attestation_id_meid;
    ASN1_OCTET_STRING* attestation_id_manufacturer;
    ASN1_OCTET_STRING* attestation_id_model;
    ASN1_INTEGER* vendor_patchlevel;
    ASN1_INTEGER* boot_patchlevel;
    ASN1_NULL* device_unique_attestation;
    ASN1_NULL* identity_credential;
    ASN1_OCTET_STRING* attestation_id_second_imei;
} KM_AUTH_LIST;

ASN1_SEQUENCE(KM_AUTH_LIST) = {
        ASN1_EXP_SET_OF_OPT(KM_AUTH_LIST, purpose, ASN1_INTEGER, TAG_PURPOSE.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, algorithm, ASN1_INTEGER, TAG_ALGORITHM.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, key_size, ASN1_INTEGER, TAG_KEY_SIZE.maskedTag()),
        ASN1_EXP_SET_OF_OPT(KM_AUTH_LIST, digest, ASN1_INTEGER, TAG_DIGEST.maskedTag()),
        ASN1_EXP_SET_OF_OPT(KM_AUTH_LIST, padding, ASN1_INTEGER, TAG_PADDING.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, ec_curve, ASN1_INTEGER, TAG_EC_CURVE.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, rsa_public_exponent, ASN1_INTEGER,
                     TAG_RSA_PUBLIC_EXPONENT.maskedTag()),
        ASN1_EXP_SET_OF_OPT(KM_AUTH_LIST, mgf_digest, ASN1_INTEGER,
                            TAG_RSA_OAEP_MGF_DIGEST.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, rollback_resistance, ASN1_NULL,
                     TAG_ROLLBACK_RESISTANCE.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, early_boot_only, ASN1_NULL, TAG_EARLY_BOOT_ONLY.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, active_date_time, ASN1_INTEGER, TAG_ACTIVE_DATETIME.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, origination_expire_date_time, ASN1_INTEGER,
                     TAG_ORIGINATION_EXPIRE_DATETIME.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, usage_expire_date_time, ASN1_INTEGER,
                     TAG_USAGE_EXPIRE_DATETIME.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, usage_count_limit, ASN1_INTEGER,
                     TAG_USAGE_COUNT_LIMIT.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, no_auth_required, ASN1_NULL, TAG_NO_AUTH_REQUIRED.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, user_auth_type, ASN1_INTEGER, TAG_USER_AUTH_TYPE.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, auth_timeout, ASN1_INTEGER, TAG_AUTH_TIMEOUT.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, allow_while_on_body, ASN1_NULL,
                     TAG_ALLOW_WHILE_ON_BODY.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, trusted_user_presence_required, ASN1_NULL,
                     TAG_TRUSTED_USER_PRESENCE_REQUIRED.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, trusted_confirmation_required, ASN1_NULL,
                     TAG_TRUSTED_CONFIRMATION_REQUIRED.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, unlocked_device_required, ASN1_NULL,
                     TAG_UNLOCKED_DEVICE_REQUIRED.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, creation_date_time, ASN1_INTEGER,
                     TAG_CREATION_DATETIME.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, origin, ASN1_INTEGER, TAG_ORIGIN.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, root_of_trust, KM_ROOT_OF_TRUST, TAG_ROOT_OF_TRUST.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, os_version, ASN1_INTEGER, TAG_OS_VERSION.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, os_patchlevel, ASN1_INTEGER, TAG_OS_PATCHLEVEL.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_application_id, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_APPLICATION_ID.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_brand, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_BRAND.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_device, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_DEVICE.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_product, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_PRODUCT.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_serial, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_SERIAL.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_imei, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_IMEI.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_meid, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_MEID.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_manufacturer, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_MANUFACTURER.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_model, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_MODEL.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, vendor_patchlevel, ASN1_INTEGER,
                     TAG_VENDOR_PATCHLEVEL.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, boot_patchlevel, ASN1_INTEGER, TAG_BOOT_PATCHLEVEL.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, device_unique_attestation, ASN1_NULL,
                     TAG_DEVICE_UNIQUE_ATTESTATION.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, identity_credential, ASN1_NULL,
                     TAG_IDENTITY_CREDENTIAL_KEY.maskedTag()),
        ASN1_EXP_OPT(KM_AUTH_LIST, attestation_id_second_imei, ASN1_OCTET_STRING,
                     TAG_ATTESTATION_ID_SECOND_IMEI.maskedTag()),
} ASN1_SEQUENCE_END(KM_AUTH_LIST);
DECLARE_ASN1_FUNCTIONS(KM_AUTH_LIST);

typedef struct km_key_description {
    ASN1_INTEGER* attestation_version;
    ASN1_ENUMERATED* attestation_security_level;
    ASN1_INTEGER* keymint_version;
    ASN1_ENUMERATED* keymint_security_level;
    ASN1_OCTET_STRING* attestation_challenge;
    ASN1_INTEGER* unique_id;
    KM_AUTH_LIST* software_enforced;
    KM_AUTH_LIST* tee_enforced;
} KM_KEY_DESCRIPTION;

ASN1_SEQUENCE(KM_KEY_DESCRIPTION) = {
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, attestation_version, ASN1_INTEGER),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, attestation_security_level, ASN1_ENUMERATED),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, keymint_version, ASN1_INTEGER),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, keymint_security_level, ASN1_ENUMERATED),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, attestation_challenge, ASN1_OCTET_STRING),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, unique_id, ASN1_OCTET_STRING),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, software_enforced, KM_AUTH_LIST),
        ASN1_SIMPLE(KM_KEY_DESCRIPTION, tee_enforced, KM_AUTH_LIST),
} ASN1_SEQUENCE_END(KM_KEY_DESCRIPTION);
DECLARE_ASN1_FUNCTIONS(KM_KEY_DESCRIPTION);

enum class VerifiedBoot : uint8_t {
    VERIFIED = 0,
    SELF_SIGNED = 1,
    UNVERIFIED = 2,
    FAILED = 3,
};

struct RootOfTrust {
    SecurityLevel security_level;
    vector<uint8_t> verified_boot_key;
    vector<uint8_t> verified_boot_hash;
    VerifiedBoot verified_boot_state;
    bool device_locked;
};

struct AttestationRecord {
    RootOfTrust root_of_trust;
    uint32_t attestation_version;
    SecurityLevel attestation_security_level;
    uint32_t keymint_version;
    SecurityLevel keymint_security_level;
    std::vector<uint8_t> attestation_challenge;
    AuthorizationSet software_enforced;
    AuthorizationSet hardware_enforced;
    std::vector<uint8_t> unique_id;
};

ErrorCode parse_attestation_record(const uint8_t* asn1_key_desc, size_t asn1_key_desc_len,
                                   uint32_t* attestation_version,  //
                                   SecurityLevel* attestation_security_level,
                                   uint32_t* keymint_version, SecurityLevel* keymint_security_level,
                                   std::vector<uint8_t>* attestation_challenge,
                                   AuthorizationSet* software_enforced,
                                   AuthorizationSet* tee_enforced,  //
                                   std::vector<uint8_t>* unique_id);

ErrorCode parse_root_of_trust(const uint8_t* asn1_key_desc, size_t asn1_key_desc_len,
                              std::vector<uint8_t>* verified_boot_key,
                              VerifiedBoot* verified_boot_state, bool* device_locked,
                              std::vector<uint8_t>* verified_boot_hash);

}  // namespace aidl::android::hardware::security::keymint
