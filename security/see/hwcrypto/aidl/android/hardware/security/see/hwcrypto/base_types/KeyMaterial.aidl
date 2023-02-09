package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcKey;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrAes;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrHmac;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrRsa;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrTDes;

union KeyMaterial {
    OpaqueOrAes Aes;
    OpaqueOrTDes TripleDes;
    OpaqueOrHmac Hmac;
    OpaqueOrRsa Rsa;
    OpaqueOrEc Ec;
}
