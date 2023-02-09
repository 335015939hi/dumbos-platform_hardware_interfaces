package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcSignParameters;
import android.hardware.security.see.hwcrypto.base_types.RsaSignParameters;

union AsymmetricSignParameters {
    RsaSignParameters Rsa;
    EcSignParameters Ec;
}
