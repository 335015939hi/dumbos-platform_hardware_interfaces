///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.security.keymint;
@VintfStability
interface IKeyMintDevice {
  android.hardware.security.keymint.KeyMintHardwareInfo getHardwareInfo();
  android.hardware.security.keymint.VerificationToken verifyAuthorization(in long challenge, in android.hardware.security.keymint.HardwareAuthToken token);
  void addRngEntropy(in byte[] data);
  @nullable android.hardware.security.keymint.CertificateChain generateKey(in android.hardware.security.keymint.KeyParameter[] keyParams, in @nullable byte[] attestationSigningKeyBlob, out android.hardware.security.keymint.ByteArray generatedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics generatedKeyCharacteristics);
  @nullable android.hardware.security.keymint.CertificateChain importKey(in android.hardware.security.keymint.KeyParameter[] keyParams, in android.hardware.security.keymint.KeyFormat keyFormat, in byte[] keyData, in @nullable byte[] attestationSigningKeyBlob, out android.hardware.security.keymint.ByteArray importedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics keyCharacteristics);
  @nullable android.hardware.security.keymint.CertificateChain importWrappedKey(in byte[] wrappedKeyData, in byte[] wrappingKeyBlob, in byte[] maskingKey, in android.hardware.security.keymint.KeyParameter[] unwrappingParams, in long passwordSid, in long biometricSid, in @nullable byte[] attestationSigningKeyBlob, out android.hardware.security.keymint.ByteArray importedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics keyCharacteristics);
  byte[] upgradeKey(in byte[] inKeyBlobToUpgrade, in android.hardware.security.keymint.KeyParameter[] inUpgradeParams);
  void deleteKey(in byte[] inKeyBlob);
  void deleteAllKeys();
  void destroyAttestationIds();
  android.hardware.security.keymint.BeginResult begin(in android.hardware.security.keymint.KeyPurpose inPurpose, in byte[] inKeyBlob, in android.hardware.security.keymint.KeyParameter[] inParams, in android.hardware.security.keymint.HardwareAuthToken inAuthToken);
  const int AUTH_TOKEN_MAC_LENGTH = 32;
}
