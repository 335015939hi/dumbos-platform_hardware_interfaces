/*
 * Copyright 2023 The Android Open Source Project
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
#include <algorithm>
#include <array>
#include <optional>
#include <vector>

#include <android-base/logging.h>
#include <cppbor.h>
#include <cppbor_parse.h>

#include "IMDocCredentialSupport.h"

namespace aidl::android::security::identity::direct_access::support {
using cppbor::MajorType::ARRAY;
using cppbor::MajorType::BSTR;
using cppbor::MajorType::SEMANTIC;
using cppbor::MajorType::SIMPLE;
using cppbor::MajorType::TSTR;
using cppbor::MajorType::UINT;
using std::string;
using std::vector;

constexpr bool MANDATORY = true;
constexpr bool OPTIONAL = false;

const std::vector<std::string> kEyeColors = {"black", "blue",  "brown",  "dichromatic", "grey",
                                             "green", "hazel", "maroon", "pink",        "unknown"};

const std::vector<std::string> kHairColors = {"bald", "black",  "blond", "brown", "grey",
                                              "red",  "auburn", "sandy", "white", "unknown"};

const std::vector<uint32_t> kSexCodes = {0 /* Not Known*/, 1 /*Male*/, 2 /* Female*/,
                                         9 /* Not Applicable*/};

// CredentialData constants
constexpr char kDocType[] = "org.iso.18013.5.1.mdl";
constexpr char kNameSpace[] = "org.iso.18013.5.1";
constexpr char kDocTypeKey[] = "docType";
constexpr char kDigestIdMapping[] = "digestIdMapping";
constexpr char kDigestId[] = "digestID";
constexpr char kRandom[] = "random";
constexpr char kIssuerAuth[] = "issuerAuth";
constexpr char kReaderAccess[] = "readerAccess";
constexpr char kVehicleCategoryCode[] = "vehicle_category_code";
constexpr char kElementIdentifier[] = "elementIdentifier";
constexpr char kElementValue[] = "elementValue";
constexpr char kVehicleCodes[] = "codes";
constexpr char kVehicleCode[] = "code";
constexpr char kVehicleCodeSign[] = "sign";
constexpr char kVehicleCodeValue[] = "value";
constexpr uint32_t kBstrTag = 24;
constexpr uint32_t kFullDateTag = 1004;
constexpr int kCoseSign1EntryCount = 4;
constexpr int kCoseSign1ProtectedParams = 0;
constexpr int kCoseSign1UnprotectedParams = 1;
constexpr int kCoseSign1Payload = 2;
constexpr int kCoseSign1Signature = 3;
constexpr int kCredentialDataEntryCount = 4;
// mDL Identifiers
constexpr char kFamilyName[] = "family_name";
constexpr char kGivenName[] = "given_name";
constexpr char kBirthDate[] = "birth_date";
constexpr char kIssueDate[] = "issue_date";
constexpr char kExpiryDate[] = "expiry_date";
constexpr char kIssuingCountry[] = "issuing_country";
constexpr char kIssuingAuthority[] = "issuing_authority";
constexpr char kDocumentNumber[] = "document_number";
constexpr char kPotrait[] = "portrait";
constexpr char kDrivingPrivileges[] = "driving_privileges";
constexpr char kUnDistinguishingSign[] = "un_distinguishing_sign";
constexpr char kAdministrativeNumber[] = "administrative_number";
constexpr char kSex[] = "sex";
constexpr char kHeight[] = "height";
constexpr char kWeight[] = "weight";
constexpr char kEyeColor[] = "eye_color";
constexpr char kHairColor[] = "hair_color";
constexpr char kBirthPlace[] = "birth_place";
constexpr char kResidentAddress[] = "resident_address";
constexpr char kPortraitCaptureDate[] = "portrait_capture_date";
constexpr char kAgeInYears[] = "age_in_years";
constexpr char kAgeBirthYear[] = "age_birth_year";
constexpr char kAgeOverNN[] = "age_over_NN";
constexpr char kIssuingJurisdiction[] = "issuing_jurisdiction";
constexpr char kNationality[] = "nationality";
constexpr char kResidentCity[] = "resident_city";
constexpr char kResidentState[] = "resident_state";
constexpr char kResidentPostalCode[] = "resident_postal_code";
constexpr char kResidentCountry[] = "resident_country";
constexpr char kBiometricTemplateXX[] = "biometric_template_xx";
constexpr char kFNNationalCharacter[] = "family_name_national_character";
constexpr char kGNNationalCharacter[] = "given_name_national_character";
constexpr char kSignatureUsualMark[] = "signature_usual_mark";

/**
 * Validator function pointer to validate a specific data element.
 */
typedef bool (*DataElementValidator)(const std::unique_ptr<cppbor::Item>& elementValueItem);

// Below are the different validator functions corresponding to a specific data element.
bool isSexCodeValid(const std::unique_ptr<cppbor::Item>& elementValueItem);
bool isEyeColorValid(const std::unique_ptr<cppbor::Item>& elementValueItem);
bool isHairColorValid(const std::unique_ptr<cppbor::Item>& elementValueItem);
bool isDrivingPrivilegeValid(const std::unique_ptr<cppbor::Item>& elementValueItem);
bool isDateTagValid(const std::unique_ptr<cppbor::Item>& elementValueItem);

struct DataElement {
    /* Element identifier name */
    std::string identifier;
    /* Encoding format of the element value */
    cppbor::MajorType encodingFormat;
    /* Optional maximum length in characters */
    std::optional<uint32_t> maxLength;
    /* Optional validator function for the specific mDL data element */
    std::optional<DataElementValidator> dataElementValidator;
    /* The presence of this data element (mandatory/optional) */
    bool presence;  // TODO remove this field if this is not validated.
};

// mDL data elements as specified in ISO/IEC FDIS 18013-5:2021(E) Table 5, section 7.2.1
const std::vector<DataElement> dataElementIdentifiers = {
        {kFamilyName, TSTR, 150, std::nullopt, MANDATORY},
        {kGivenName, TSTR, 150, std::nullopt, MANDATORY},
        {kBirthDate, SEMANTIC, std::nullopt, isDateTagValid, MANDATORY},
        {kIssueDate, SEMANTIC, std::nullopt, isDateTagValid, MANDATORY},
        {kExpiryDate, SEMANTIC, std::nullopt, isDateTagValid, MANDATORY},
        {kIssuingCountry, TSTR, 2, std::nullopt, MANDATORY},
        {kIssuingAuthority, TSTR, 150, std::nullopt, MANDATORY},
        {kDocumentNumber, TSTR, 150, std::nullopt, MANDATORY},
        {kPotrait, BSTR, std::nullopt, std::nullopt, MANDATORY},
        {kDrivingPrivileges, ARRAY, std::nullopt, isDrivingPrivilegeValid, MANDATORY},
        {kUnDistinguishingSign, TSTR, std::nullopt, std::nullopt, MANDATORY},
        {kAdministrativeNumber, TSTR, 150, std::nullopt, OPTIONAL},
        {kSex, UINT, std::nullopt, isSexCodeValid, OPTIONAL},
        {kHeight, UINT, std::nullopt, std::nullopt, OPTIONAL},
        {kWeight, UINT, std::nullopt, std::nullopt, OPTIONAL},
        {kEyeColor, TSTR, std::nullopt, isEyeColorValid, OPTIONAL},
        {kHairColor, TSTR, std::nullopt, isHairColorValid, OPTIONAL},
        {kBirthPlace, TSTR, 150, std::nullopt, OPTIONAL},
        {kResidentAddress, TSTR, 150, std::nullopt, OPTIONAL},
        {kPortraitCaptureDate, SEMANTIC, std::nullopt, isDateTagValid, OPTIONAL},
        {kAgeInYears, UINT, std::nullopt, std::nullopt, OPTIONAL},
        {kAgeBirthYear, UINT, std::nullopt, std::nullopt, OPTIONAL},
        {kAgeOverNN, SIMPLE, std::nullopt, std::nullopt, OPTIONAL},
        {kIssuingJurisdiction, TSTR, std::nullopt, std::nullopt, OPTIONAL},
        {kNationality, TSTR, 2, std::nullopt, OPTIONAL},
        {kResidentCity, TSTR, 150, std::nullopt, OPTIONAL},
        {kResidentState, TSTR, 150, std::nullopt, OPTIONAL},
        {kResidentPostalCode, TSTR, 150, std::nullopt, OPTIONAL},
        {kResidentCountry, TSTR, 2, std::nullopt, OPTIONAL},
        {kBiometricTemplateXX, BSTR, std::nullopt, std::nullopt, OPTIONAL},
        {kFNNationalCharacter, TSTR, std::nullopt, std::nullopt, OPTIONAL},
        {kGNNationalCharacter, TSTR, std::nullopt, std::nullopt, OPTIONAL},
        {kSignatureUsualMark, BSTR, std::nullopt, std::nullopt, OPTIONAL}};

bool isElementIdentifierValid(const DataElement& dataElement, const std::string& elementIdentifier,
                              const std::unique_ptr<cppbor::Item>& elementValueItem) {
    if (elementIdentifier != dataElement.identifier) {
        return false;
    }
    // For SEMANTIC tags elementValueItem->type() gives the type of the taggedItem and not
    // SEMANTIC tag itself.
    if ((dataElement.encodingFormat != SEMANTIC) &&
        (dataElement.encodingFormat != elementValueItem->type())) {
        return false;
    }
    if (dataElement.maxLength.has_value()) {
        const cppbor::Tstr* tstrVal = elementValueItem->asTstr();
        if (tstrVal == nullptr) {
            return false;
        }
        if (tstrVal->value().size() > dataElement.maxLength.value()) {
            return false;
        }
    }
    if (dataElement.dataElementValidator.has_value()) {
        return dataElement.dataElementValidator.value()(elementValueItem);
    }
    return true;
}

bool validateIssuerSignedItemBytesArray(
        const std::unique_ptr<cppbor::Item>& issuerSignedItemBytesArrayItem) {
    if (issuerSignedItemBytesArrayItem == nullptr) {
        LOG(ERROR) << "digestIdMapping doesn't contain issuerSignedItems";
        return false;
    }
    const cppbor::Array* issuerSignedItemBytesArray = issuerSignedItemBytesArrayItem->asArray();
    if (issuerSignedItemBytesArray == nullptr) {
        LOG(ERROR) << "issuerSignedItems must be an array";
        return false;
    }

    for (auto it = issuerSignedItemBytesArray->begin(); it != issuerSignedItemBytesArray->end();
         ++it) {
        const cppbor::SemanticTag* tag = (*it)->asSemanticTag();
        if (tag == nullptr) {
            LOG(ERROR) << "issuerSignedItem must be a semantic tag";
            return false;
        }
        if (tag->semanticTag() != kBstrTag || tag->type() != BSTR) {
            LOG(ERROR) << "The issuerSignedItem tag must be a bstr";
            return false;
        }
        const cppbor::Bstr* issuerSignedItemBstr = tag->asBstr();
        if (issuerSignedItemBstr == nullptr) {
            LOG(ERROR) << "The issuerSignedItem tag must be a bstr";
            return false;
        }
        auto [item, _, message] = cppbor::parse(issuerSignedItemBstr->value());
        if (item == nullptr) {
            LOG(ERROR) << "Passed-in issuerSignedItem is not valid CBOR: " << message;
            return false;
        }
        const cppbor::Map* issuerSignedItemMap = item->asMap();
        if (issuerSignedItemMap == nullptr) {
            LOG(ERROR) << "issuerSignedItem's taggedItem must be a map";
            return false;
        }
        const auto& digestIdItem = issuerSignedItemMap->get(kDigestId);
        if (digestIdItem == nullptr) {
            LOG(ERROR) << "issuerSignedItem map must contain digestId";
            return false;
        }
        const cppbor::Uint* digestId = digestIdItem->asUint();
        if (digestId == nullptr) {
            LOG(ERROR) << "digestId must be an uint";
            return false;
        }
        const auto& randomItem = issuerSignedItemMap->get(kRandom);
        if (randomItem == nullptr) {
            LOG(ERROR) << "issuerSignedItem map must contain ramdom";
            return false;
        }
        const cppbor::Bstr* random = randomItem->asBstr();
        if (random == nullptr) {
            LOG(ERROR) << "random must be a bstr";
            return false;
        }
        const auto& elementIdentifierItem = issuerSignedItemMap->get(kElementIdentifier);
        if (elementIdentifierItem == nullptr) {
            LOG(ERROR) << "issuerSignedItem map must contain elementIdentifier";
            return false;
        }
        const cppbor::Tstr* elementIdentifier = elementIdentifierItem->asTstr();
        if (elementIdentifier == nullptr) {
            LOG(ERROR) << "elementIdentifier must be a tstr";
            return false;
        }
        const auto& elementValueItem = issuerSignedItemMap->get(kElementValue);
        if (elementValueItem == nullptr) {
            LOG(ERROR) << "issuerSignedItem map must contain elementValue";
            return false;
        }
        // Validate the elementIdentifier and the elementValue
        const std::string& elementIdentifierStr = elementIdentifier->value();
        auto itr = std::find_if(
                dataElementIdentifiers.begin(), dataElementIdentifiers.end(),
                [&elementIdentifierStr, &elementValueItem](const DataElement& dataElement) {
                    return isElementIdentifierValid(dataElement, elementIdentifierStr,
                                                    elementValueItem);
                });
        if (itr == dataElementIdentifiers.end()) {
            LOG(ERROR) << "Invalid CBOR encoding for identifier: " << elementIdentifierStr;
            return false;
        }
    }
    return true;
}

bool validateIssuerAuth(const std::unique_ptr<cppbor::Item>& issuerAuthItem) {
    if (issuerAuthItem == nullptr) {
        LOG(ERROR) << "CredentialData must contain issuerAuth";
        return false;
    }
    const cppbor::Array* issuerAuth = issuerAuthItem->asArray();
    if (issuerAuth == nullptr) {
        LOG(ERROR) << "issuerAuth must be an array";
        return false;
    }
    if (issuerAuth->size() != kCoseSign1EntryCount) {
        LOG(ERROR) << "issuerAuth array must be CoseSign1";
        return false;
    }
    const cppbor::Bstr* protectedParams = issuerAuth->get(kCoseSign1ProtectedParams)->asBstr();
    const cppbor::Map* unprotectedParams = issuerAuth->get(kCoseSign1UnprotectedParams)->asMap();
    const cppbor::Bstr* payload = issuerAuth->get(kCoseSign1Payload)->asBstr();
    const cppbor::Bstr* signature = issuerAuth->get(kCoseSign1Signature)->asBstr();

    if (!protectedParams || !unprotectedParams || !payload || !signature) {
        LOG(ERROR) << "issuerAuth CoseSign1 is missing input parameters";
        return false;
    }
    return true;
}

bool validateReaderAccess(const std::unique_ptr<cppbor::Item>& readerAccessItem) {
    if (readerAccessItem == nullptr) {
        LOG(ERROR) << "CredentialData must contain readAccess";
        return false;
    }
    const cppbor::Array* array = readerAccessItem->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "readerAccess must be an array";
        return false;
    }
    if (array->size() > 0) {
        for (auto it = array->begin(); it != array->end(); ++it) {
            if ((*it) == nullptr) {
                LOG(ERROR) << "readerAccess must contain coseKey";
                return false;
            }
            const cppbor::Map* map = (*it)->asMap();
            if (map == nullptr) {
                LOG(ERROR) << "coseKey in the readerAccess must be a map";
                return false;
            }
        }
    }
    return true;
}

/**
 * CredentialData = {
 *      "docType": "org.iso.18013.5.1.mdl",
 *      "digestIdMapping": DigestIdMapping,
 *      "issuerAuth" : IssuerAuth,
 *      "readerAccess" : ReaderAccess
 * }
 *
 * IssuerAuth = COSE_Sign1  ; payload as mentioned in
 * ISO-IECJTC1-SC17-WG10_N1985_ISO_IEC_FDIS_18013-5_E.pdf
 *
 * DigestIdMapping = {
 *      NameSpace => [ + IssuerSignedItemBytes ]
 * }
 *
 * NameSpace = "org.iso.18013.5.1"
 *
 * ReaderAccess = [ * COSE_Key ]
 *
 * IssuerSignedItemBytes = #6.24(bstr .cbor IssuerSignedItem)
 *
 * IssuerSignedItem = {
 *   "digestID" : uint, ; Digest ID for issuer data authentication
 *   "random" : bstr, ; Random value for issuer data authentication
 *   "elementIdentifier" : DataElementIdentifier, ; Data element identifier
 *   "elementValue" : DataElementValue ; Data element value
 * }
 *
 * DataElementIdentifier: tstr
 *
 */
bool validateCredentialData(const std::vector<uint8_t>& credentialData) {
    auto [item, _, message] = cppbor::parse(credentialData);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in credentialData is not valid CBOR: " << message;
        return false;
    }
    const cppbor::Map* map = item->asMap();
    if (map == nullptr) {
        LOG(ERROR) << "CredentialData must be a map";
        return false;
    }
    if (map->size() != kCredentialDataEntryCount) {
        LOG(ERROR) << "CredentialData must have a size of 4";
        return false;
    }
    const auto& docTypeItem = map->get(kDocTypeKey);
    if (docTypeItem == nullptr) {
        LOG(ERROR) << "CredentialData must contain docType";
        return false;
    }
    const cppbor::Tstr* docTypeTstr = docTypeItem->asTstr();
    if (docTypeTstr == nullptr) {
        LOG(ERROR) << "docType in the CredentialData must be a tstr";
        return false;
    }
    if (!strncmp(kDocType, docTypeTstr->value().data(), strlen(kDocType))) {
        LOG(ERROR) << "docType contains invalid identifier";
        return false;
    }
    const auto& digestIdMappingItem = map->get(kDigestIdMapping);
    if (digestIdMappingItem == nullptr) {
        LOG(ERROR) << "CredentialData must contain digestIdMapping";
        return false;
    }
    const cppbor::Map* digestIdMappingMap = digestIdMappingItem->asMap();
    if (digestIdMappingMap == nullptr) {
        LOG(ERROR) << "digestIdMapping in the CredentialData must be a map";
        return false;
    }
    const auto& issuerSignedItemBytesArrayItem = digestIdMappingMap->get(kNameSpace);
    if (!validateIssuerSignedItemBytesArray(issuerSignedItemBytesArrayItem)) {
        return false;
    }
    const auto& isserAuthItem = map->get(kIssuerAuth);
    if (!validateIssuerAuth(isserAuthItem)) {
        return false;
    }
    const auto& issreaderAccessItem = map->get(kReaderAccess);
    if (!validateReaderAccess(issreaderAccessItem)) {
        return false;
    }
    return true;
}

bool isSexCodeValid(const std::unique_ptr<cppbor::Item>& elementValueItem) {
    const cppbor::Uint* intVal = elementValueItem->asUint();
    if (intVal == nullptr) {
        return false;
    }
    auto it = std::find_if(kSexCodes.begin(), kSexCodes.end(),
                           [&intVal](const uint32_t& val) { return val != intVal->value(); });

    if (it == kSexCodes.end()) {
        return false;
    }
    return true;
}

bool isEyeColorValid(const std::unique_ptr<cppbor::Item>& elementValueItem) {
    const cppbor::Tstr* tstrVal = elementValueItem->asTstr();
    if (tstrVal == nullptr) {
        return false;
    }
    auto it = std::find_if(kEyeColors.begin(), kEyeColors.end(),
                           [&tstrVal](const std::string& val) { return val != tstrVal->value(); });
    if (it == kEyeColors.end()) {
        return false;
    }
    return true;
}

bool isHairColorValid(const std::unique_ptr<cppbor::Item>& elementValueItem) {
    const cppbor::Tstr* tstrVal = elementValueItem->asTstr();
    if (tstrVal == nullptr) {
        return false;
    }
    auto it = std::find_if(kHairColors.begin(), kHairColors.end(),
                           [&tstrVal](const std::string& val) { return val != tstrVal->value(); });
    if (it == kHairColors.end()) {
        return false;
    }
    return true;
}

bool isDateTagValid(const std::unique_ptr<cppbor::Item>& elementValueItem) {
    if (!elementValueItem || elementValueItem->semanticTagCount() != 1) {
        return false;
    }
    const cppbor::SemanticTag* tag = elementValueItem->asSemanticTag();
    if (!tag || (tag->semanticTag() != kFullDateTag) || (tag->type() != TSTR)) {
        return false;
    }
    return true;
}

/**
 * DrivingPrivileges = [
 *    * DrivingPrivilege
 * ]
 *
 * DrivingPrivilege = {
 *    "vehicle_category_code" : tstr ; Vehicle category code as per ISO/IEC 18013-1 Annex B
 *    ? "issue_date" : full-date ; Date of issue encoded as full-date
 *    ? "expiry_date" : full-date ; Date of expiry encoded as full-date
 *    ? "codes" : [+Code] ; Array of code info
 * }
 *
 * Code = {
 *    "code": tstr ; Code as per ISO/IEC 18013-2 Annex A
 *    ? "sign": tstr ; Sign as per ISO/IEC 18013-2 Annex A
 *    ? "value": tstr ; Value as per ISO/IEC 18013-2 Annex A
 * }
 *
 */
bool isDrivingPrivilegeValid(const std::unique_ptr<cppbor::Item>& elementValueItem) {
    const cppbor::Array* privileges = elementValueItem->asArray();
    if (privileges == nullptr) {
        LOG(ERROR) << "DrivingPrivileges must be an array";
        return false;
    }
    if (privileges->size() > 0) {
        for (auto it = privileges->begin(); it != privileges->end(); ++it) {
            if ((*it) == nullptr) {
                LOG(ERROR) << "DrivingPrivilege must contain a non-null item";
                return false;
            }
            const cppbor::Map* drivingPrivilegeMap = (*it)->asMap();
            if (drivingPrivilegeMap == nullptr) {
                LOG(ERROR) << "DrivingPrivilege must be a map";
                return false;
            }
            const auto& vCategoryCodeItem = drivingPrivilegeMap->get(kVehicleCategoryCode);
            if (vCategoryCodeItem == nullptr) {
                LOG(ERROR) << "DrivingPrivilege map must contain vehicle_category_code";
            }
            const cppbor::Tstr* vCategoryCode = vCategoryCodeItem->asTstr();
            if (vCategoryCode == nullptr) {
                LOG(ERROR) << "Vehicle category code must be a tstr.";
            }
            // issue_date is optional item
            const auto& vIssueDateItem = drivingPrivilegeMap->get(kIssueDate);
            if (vIssueDateItem && !isDateTagValid(vIssueDateItem)) {
                LOG(ERROR) << "The issueDate tag must be a tstr with tag: " << kFullDateTag;
                return false;
            }
            // expiry_date is optional item
            const auto& vExpiryDateItem = drivingPrivilegeMap->get(kExpiryDate);
            if (vExpiryDateItem && !isDateTagValid(vExpiryDateItem)) {
                LOG(ERROR) << "The expiryDate tag must be a tstr with tag: " << kFullDateTag;
                return false;
            }
            // codes is optional
            const auto& vCodesItem = drivingPrivilegeMap->get(kVehicleCodes);
            if (vCodesItem != nullptr) {
                const cppbor::Array* array = vCodesItem->asArray();
                if (array == nullptr || array->size() == 0) {
                    LOG(ERROR) << "Vehicle codes must be an array with size > 0";
                    return false;
                }
                for (auto codesItr = array->begin(); codesItr != array->end(); ++codesItr) {
                    if ((*codesItr) == nullptr) {
                        LOG(ERROR) << "Vehicle code array must contain a non-null item";
                        return false;
                    }
                    const cppbor::Map* codeMap = (*codesItr)->asMap();
                    if (codeMap == nullptr) {
                        LOG(ERROR) << "vehicle code item must be a map";
                        return false;
                    }
                    const auto& codeTstrItem = codeMap->get(kVehicleCode);
                    if (codeTstrItem == nullptr) {
                        LOG(ERROR) << "vehicle code item must contain a code";
                        return false;
                    }
                    const cppbor::Tstr* codeTstr = codeTstrItem->asTstr();
                    if (codeTstr == nullptr) {
                        LOG(ERROR) << "vehicle code item must be a tstr";
                        return false;
                    }
                    // optional
                    const auto& signTstrItem = codeMap->get(kVehicleCodeSign);
                    if (signTstrItem && !signTstrItem->asTstr()) {
                        LOG(ERROR) << "Vehicle code sign must be a tstr.";
                        return false;
                    }
                    // optional
                    const auto& valueTstrItem = codeMap->get(kVehicleCodeValue);
                    if (valueTstrItem && !valueTstrItem->asTstr()) {
                        LOG(ERROR) << "Vehicle code value must be tstr.";
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

}  // namespace aidl::android::security::identity::direct_access::support
