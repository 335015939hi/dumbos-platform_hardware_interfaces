package android.hardware.security.see.hwcrypto.base_types;
// TODO: See if we should use the current definition instead where everythign is a binary blob that
//       can be decoded
parcelable DiceArtifact {
    byte[] CdiAttest;
    byte[] CdiSeal;
    @nullable byte[] Certificate;
}
