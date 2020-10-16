///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.identity;
@VintfStability
interface IWritableIdentityCredential {
  android.hardware.identity.Certificate[] getAttestationCertificate(in byte[] attestationApplicationId, in byte[] attestationChallenge);
  void startPersonalization(in int accessControlProfileCount, in int[] entryCounts);
  android.hardware.identity.SecureAccessControlProfile addAccessControlProfile(in int id, in android.hardware.identity.Certificate readerCertificate, in boolean userAuthenticationRequired, in long timeoutMillis, in long secureUserId);
  void beginAddEntry(in int[] accessControlProfileIds, in @utf8InCpp String nameSpace, in @utf8InCpp String name, in int entrySize);
  byte[] addEntryValue(in byte[] content);
  void finishAddingEntries(out byte[] credentialData, out byte[] proofOfProvisioningSignature);
  void setExpectedProofOfProvisioningSize(in int expectedProofOfProvisioningSize);
}
