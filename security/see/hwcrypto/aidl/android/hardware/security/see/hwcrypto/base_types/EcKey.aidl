package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcCurve;
import android.hardware.security.see.hwcrypto.base_types.EcCurveType;
import android.hardware.security.see.hwcrypto.base_types.EcKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.OpaqueOrEc;

// TODO: check this change. On KM, EcCurve and EcCurveType are available before
//       the OpaqueOr level, here we only have it for explicit key. I believe this
//       change should be fine in the sense that if we have an Opaque key; the
//       users should not usually need this information, but if it is needed a function
//       to get more data about a key could be better for that.

parcelable EcKey {
    EcCurve Curve;
    EcCurveType Type;
    EcKeyMaterial keyMaterial;
}
