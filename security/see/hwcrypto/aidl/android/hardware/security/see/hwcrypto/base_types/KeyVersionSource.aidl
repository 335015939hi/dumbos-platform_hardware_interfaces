package android.hardware.security.see.hwcrypto.base_types;

/*
 * enum KeyVersionSource - Rollback version source.
 *
 * @CommittedVersion:
 *     Gate the derived key based on the anti-rollback counter that has been
 *     committed to fuses or stored. A component with a version smaller
 *     than this value should never run on the device again. The latest key may
 *     not be available the first few times a new version of the component runs on the
 *     device, because the counter may not be committed immediately. This
 *     version source may not allow versions > 0 on some devices (i.e. rollback
 *     versions cannot be committed).
 * @RunningVersion:
 *     Gate the derived key based on the anti-rollback version in the signed
 *     image of the component that is currently running. The latest key should be
 *     available immediately, but the component may be rolled back on a
 *     future boot. Care should be taken that everything still works if the image is
 *     rolled back and access to this key is lost. Care should also be taken
 *     that it is not possible to infer this key if it rolls back to a previous version.
 *     For example, storing the latest version of this key in storage
 *     would allow it to be retrieved after rollback.
 */
@Backing(type="byte")
enum KeyVersionSource {
    CommittedVersion = 0,
    RunningVersion = 1,
}
