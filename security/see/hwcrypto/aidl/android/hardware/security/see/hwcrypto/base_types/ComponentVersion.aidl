package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.VersionedComponent;

parcelable ComponentVersion {
    VersionedComponent component = VersionedComponent.SEE_VERSION;
    int version;
}
