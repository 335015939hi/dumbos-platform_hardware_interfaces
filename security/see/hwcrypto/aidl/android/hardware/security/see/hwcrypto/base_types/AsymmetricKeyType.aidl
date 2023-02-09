package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcKeyType;
import android.hardware.security.see.hwcrypto.base_types.RsaKeyType;

union AsymmetricKeyType {
    RsaKeyType Rsa;
    EcKeyType Ec;
}
