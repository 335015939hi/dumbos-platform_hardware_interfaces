# Security-Related HALs

The `security/` subdirectory holds various security-related HALs.

The most significant HAL is KeyMint (**`IKeyMintDevice`** in the
`hardware/interfaces/security/keymint/` directory), which allows access to cryptographic
functionality where the key material is restricted to a secure environment.  This functionality is
used by Android system services, and is also made available to apps via Android Keystore.

A KeyMint implementation (or an implementation of its predecessor, Keymaster) that runs in an
isolated execution environment (e.g. ARM TrustZone) is required for most Android devices; see [CDD
9.11](https://source.android.com/docs/compatibility/13/android-13-cdd#911_keys_and_credentials).

A device may optionally also support a second KeyMint instance, running in a dedicated secure
processor; this is known as StrongBox ([CDD
9.11.2](https://source.android.com/docs/compatibility/13/android-13-cdd#9112_strongbox)).

Two specific features of KeyMint are worth highlighting, as they have an impact on the other
security-related HALs:

- KeyMint supports keys that can only be used when the operation is authenticated by the user,
  either by their lock screen knowledge factor (LSKF, e.g. PIN or pattern) or by a strong biometric
  (e.g. fingerprint).
- KeyMint supports *attestation* of public keys: when an asymmetric keypair is created, the secure
  environment produces a chain of signed certificates:
  - starting from a Google-signed root certificate
  - terminating in a leaf certificate that holds the public key; this leaf certificate may also
    describe the state of the device and the policies attached to the key.

## User Authentication

User authentication must also take place in a secure environment, but the results of that
authentication are communicated to KeyMint via Android.  As such, the authentication result (a
*hardware auth token*) is signed with a per-boot HMAC key known only to the secure components.

If an authenticator, for example GateKeeper (described by the **`IGatekeeper`** HAL in
`hardware/interfaces/gatekeeper/`), is co-located in the same secure environment as KeyMint, it can
use a local, vendor-specific, method to communicate the shared HMAC key.

However, if the authenticator is in a different environment than the KeyMint instance then a local
communication mechanism may not be possible.  For example, a StrongBox KeyMint instance running in a
separate secure processor may not have a communication channel with a TEE on the main processor.

To allow for this, the **`ISharedSecret`** HAL (in `hardware/interfaces/security/sharedsecret`)
describes an N-party shared key agreement protocol for per-boot derivation of the shared HMAC key,
based on a pre-provisioned shared secret.  This HAL can be implemented by any security component
&ndash; whether KeyMint instance or authenticator &ndash; that needs access to the shared HMAC key.

User authentication operations are also timestamped, but a StrongBox KeyMint instance may not have
access to a secure time source that is aligned with the authenticator's time source.

To allow for this, the **`ISecureClock`** HAL (in `hardware/interfaces/secureclock`) describes a
challenge-based timestamp authentication protocol.  This HAL is optional; it need only be
implemented if there is a KeyMint instance without a secure source of time.

## Attestation Key Provisioning

As noted above, key generation may also generate an attestation certificate chain, which requires
that the secure environment have access to a signing key which in turn chains back to the Google
root.

Historically these signing keys were created by Google and provided to vendors for installation in
batches of devices (to prevent their use as unique device identifiers).  However, this mechanism had
significant disadvantages, as it required secure handling of key material and only allowed for
coarse-grained revocation.

The remote key provisioning HAL (**`IRemotelyProvisionedComponent`** in
`hardware/interfaces/security/rkp/`) provides a mechanism whereby signing keys for attestation can
retrieved at runtime from Google servers, based on pre-registered device identity information.
This mechanism is used to provision signing keys for KeyMint, but is not restricted to that purpose;
it can also be used in other scenarios where keys need to be provisioned.

## Keymaster

The Keymaster HAL (**`IKeymasterDevice`** in `hardware/interfaces/keymaster/`) is the historical
ancestor of many of the HALs here (and may still be present on older devices).  Its functionality is
effectively the union of the following current HALs:

- **`IKeyMintDevice`**
- **`ISharedSecret`**
- **`ISecureClock`**
