package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EmptyEnum;
import android.hardware.security.see.hwcrypto.base_types.RsaSharedSecretParameters;

union AsymmetricSharedSecretParameters {
    RsaSharedSecretParameters Rsa;
    EmptyEnum Ec;
}
