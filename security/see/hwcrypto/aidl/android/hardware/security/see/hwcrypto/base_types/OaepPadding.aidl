package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.Digest;

parcelable OaepPadding {
    Digest msg_digest = Digest.SHA256;
    Digest mgf_digest = Digest.SHA256;
}
