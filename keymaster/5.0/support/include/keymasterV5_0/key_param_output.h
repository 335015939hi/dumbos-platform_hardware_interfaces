/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_KEYMASTER_V5_0_SUPPORT_INCLUDE_KEY_PARAM_OUTPUT_H_
#define HARDWARE_INTERFACES_KEYMASTER_V5_0_SUPPORT_INCLUDE_KEY_PARAM_OUTPUT_H_

#include <iostream>
#include <vector>

#include "keymaster_tags.h"

#include <android/hardware/keymaster/Algorithm.h>
#include <android/hardware/keymaster/BlockMode.h>
#include <android/hardware/keymaster/Digest.h>
#include <android/hardware/keymaster/EcCurve.h>
#include <android/hardware/keymaster/ErrorCode.h>
#include <android/hardware/keymaster/HardwareAuthenticatorType.h>
#include <android/hardware/keymaster/KeyBlobUsageRequirements.h>
#include <android/hardware/keymaster/KeyCharacteristics.h>
#include <android/hardware/keymaster/KeyOrigin.h>
#include <android/hardware/keymaster/KeyParameter.h>
#include <android/hardware/keymaster/KeyPurpose.h>
#include <android/hardware/keymaster/PaddingMode.h>
#include <android/hardware/keymaster/SecurityLevel.h>
#include <android/hardware/keymaster/Tag.h>
#include <android/hardware/keymaster/TagType.h>

namespace android {
namespace hardware {
namespace keymaster {

using namespace ::android::hardware::keymaster;

inline ::std::ostream& operator<<(::std::ostream& os, Algorithm value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, BlockMode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, Digest value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, EcCurve value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, ErrorCode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, KeyOrigin value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, PaddingMode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, SecurityLevel value) {
    return os << toString(value);
}

template <typename ValueT>
::std::ostream& operator<<(::std::ostream& os, const NullOr<ValueT>& value) {
    if (!value.isOk()) {
        os << "(value not present)";
    } else {
        os << value.value();
    }
    return os;
}

::std::ostream& operator<<(::std::ostream& os, const ::std::vector<KeyParameter>& set);
::std::ostream& operator<<(::std::ostream& os, const KeyParameter& param);

inline ::std::ostream& operator<<(::std::ostream& os, const KeyCharacteristics& value) {
    return os << "SW: " << value.softwareEnforced << ::std::endl
              << "HW: " << value.hardwareEnforced << ::std::endl;
}

inline ::std::ostream& operator<<(::std::ostream& os, KeyPurpose value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, Tag tag) {
    return os << toString(tag);
}

}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_KEYMASTER_V5_0_SUPPORT_INCLUDE_KEY_PARAM_OUTPUT_H_
