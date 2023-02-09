package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AsymmetricKeyType;
import android.hardware.security.see.hwcrypto.base_types.HashKeyType;
import android.hardware.security.see.hwcrypto.base_types.SymmetricKeyType;

union KeyType {
    SymmetricKeyType Symmetric;
    AsymmetricKeyType Asymmetric;
    HashKeyType Hash;
}
