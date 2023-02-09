package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcCurve;
import android.hardware.security.see.hwcrypto.base_types.EcCurveType;
import android.hardware.security.see.hwcrypto.base_types.EcKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrEc;

parcelable EcKey {
    EcCurve curve = EcCurve.P256;
    EcCurveType type = EcCurveType.NIST;
    EcKeyMaterial key_material;
}
