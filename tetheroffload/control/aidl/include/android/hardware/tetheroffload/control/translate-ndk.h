// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include <limits>
#include "aidl/android/hardware/tetheroffload/control/IPv4AddrPortPair.h"
#include "aidl/android/hardware/tetheroffload/control/NatTimeoutUpdate.h"
#include "aidl/android/hardware/tetheroffload/control/NetworkProtocol.h"
#include "aidl/android/hardware/tetheroffload/control/OffloadCallbackEvent.h"
#include "android/hardware/tetheroffload/control/1.0/types.h"
#include "android/hardware/tetheroffload/control/1.1/types.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair& in,
        aidl::android::hardware::tetheroffload::control::IPv4AddrPortPair* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate& in,
        aidl::android::hardware::tetheroffload::control::NatTimeoutUpdate* out);

}  // namespace android::h2a
