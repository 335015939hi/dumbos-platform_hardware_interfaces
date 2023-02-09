package android.hardware.security.see.hwcrypto.base_types;

/*
 * enum EvictReason - Defines on which case the key should be invalidated. Notice that hardware
 *                    support might be needed to provide these guarantees.
 *
 * @SECURITY_ANCHOR_STATE_CHANGE:
 *      Keys should be invalidated if the security state of the Security anchor (e.g. GSA) changes.
 * @SECURE_ENCLAVE_STATE_CHANGE:
 *      Keys should be invalidated if the security state of the secure enclave (e.g. TZ) changes.
 * @NON_SECURE_WORLD_STATE_CHANGE:
 *      Keys should be invalidated if the security state of the non-secure world changes.
 */
enum EvictReason {
    SECURITY_ANCHOR_STATE_CHANGE,
    SECURE_ENCLAVE_STATE_CHANGE,
    NON_SECURE_WORLD_STATE_CHANGE,
}
