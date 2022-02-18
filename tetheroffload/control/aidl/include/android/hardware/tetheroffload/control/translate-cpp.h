// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include <limits>
#include "android/hardware/tetheroffload/control/1.0/types.h"
#include "android/hardware/tetheroffload/control/1.1/types.h"
#include "android/hardware/tetheroffload/control/IPv4AddrPortPair.h"
#include "android/hardware/tetheroffload/control/NatTimeoutUpdate.h"
#include "android/hardware/tetheroffload/control/NetworkProtocol.h"
#include "android/hardware/tetheroffload/control/OffloadCallbackEvent.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair& in,
        android::hardware::tetheroffload::control::IPv4AddrPortPair* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate& in,
        android::hardware::tetheroffload::control::NatTimeoutUpdate* out);

}  // namespace android::h2a
