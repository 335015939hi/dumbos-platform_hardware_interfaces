package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.NonVmDomain;

union Domain {
    NonVmDomain NonVmDomain = NonVmDomain.SECURITY_ANCHOR;
    int VmDomain;
}
