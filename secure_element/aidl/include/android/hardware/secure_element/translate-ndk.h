// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include <limits>
#include "aidl/android/hardware/secure_element/LogicalChannelResponse.h"
#include "aidl/android/hardware/secure_element/SecureElementStatus.h"
#include "android/hardware/secure_element/1.0/types.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::secure_element::V1_0::LogicalChannelResponse& in,
        aidl::android::hardware::secure_element::LogicalChannelResponse* out);

}  // namespace android::h2a
