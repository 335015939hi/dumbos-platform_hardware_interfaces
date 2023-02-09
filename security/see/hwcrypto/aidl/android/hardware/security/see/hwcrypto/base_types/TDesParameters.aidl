package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.TDesModeParameters;

// Ecb Mode do not use the TDesModeParameters, but AIDL enums/unions do not
// accpet elements with no types.
union TDesParameters {
    TDesModeParameters EcbNoPadding;
    TDesModeParameters EcbPkcs7Padding;
    TDesModeParameters CbcNoPadding;
    TDesModeParameters CbcPkcs7Padding;
}
