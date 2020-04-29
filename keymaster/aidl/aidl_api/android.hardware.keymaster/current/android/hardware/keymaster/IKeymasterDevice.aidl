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

package android.hardware.keymaster;
@VintfStability
interface IKeymasterDevice {
  void getHardwareInfo(out android.hardware.keymaster.KeymasterHardwareInfo info);
  android.hardware.keymaster.VerificationToken verifyAuthorization(in android.hardware.keymaster.IKeymasterOperation operation, in android.hardware.keymaster.KeyParameter[] parametersToVerify, in android.hardware.keymaster.HardwareAuthToken token);
  void addRngEntropy(in byte[] data);
  void generateKey(in android.hardware.keymaster.KeyParameter[] keyParams, out byte[] generatedKeyBlob, out android.hardware.keymaster.KeyCharacteristics generatedKeyCharacteristics);
  void importKey(in android.hardware.keymaster.KeyParameter[] inKeyParams, in android.hardware.keymaster.KeyFormat inKeyFormat, in byte[] inKeyData, out byte[] importedKeyBlob, out android.hardware.keymaster.KeyCharacteristics importedKeyCharacteristics);
  void importWrappedKey(in byte[] inWrappedKeyData, in byte[] inWrappingKeyBlob, in byte[] inMaskingKey, in android.hardware.keymaster.KeyParameter[] inUnwrappingParams, in long inPasswordSid, in long inBiometricSid, out byte[] importedKeyBlob, out android.hardware.keymaster.KeyCharacteristics importedKeyCharacteristics);
  android.hardware.keymaster.KeyCharacteristics getKeyCharacteristics(in byte[] keyBlob, in byte[] clientId, in byte[] appData);
  byte[] exportKey(in android.hardware.keymaster.KeyFormat keyFormat, in byte[] keyBlob, in byte[] clientId, in byte[] appData);
  android.hardware.keymaster.Certificate[] attestKey(in byte[] keyToAttest, in android.hardware.keymaster.KeyParameter[] attestParams);
  byte[] upgradeKey(in byte[] keyBlobToUpgrade, in android.hardware.keymaster.KeyParameter[] upgradeParams);
  void deleteKey(in byte[] keyBlob);
  void deleteAllKeys();
  void destroyAttestationIds();
  android.hardware.keymaster.IKeymasterOperation begin(in android.hardware.keymaster.KeyPurpose inPurpose, in byte[] inKeyBlob, in android.hardware.keymaster.KeyParameter[] inParams, in android.hardware.keymaster.HardwareAuthToken inAuthToken, out android.hardware.keymaster.KeyParameter[] outParams);
}
