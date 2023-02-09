package android.hardware.security.see.hwcrypto.base_types;

/*
 * enum KeyPermissions - Additional permissions of the key.
 *
 * @KEY_MANAGEMENT_KEY:
 *      Key can be uised to wrap or derive other keys.
 * @ALLOW_EPHEMERAL_KEY_WRAPPING:
 *      Key can be wrapped by an ephemeral key.
 * @ALLOW_HARDWARE_KEY_WRAPPING:
 *      Key can be wrapped by a hardware key. Notice that ephemeral keys cannot be wrapped by
 *      hardware keys.
 * @ALLOW_PORTABLE_KEY_WRAPPING:
 *      Key can be wrapped by a portable key. Notice that neither ephemeral keys nor hardware keys
 *      can be wrapped by portable keys.
 * @BOOTSTATE_BINDING:
 *      Key should be bind to the boot state of the platform. This allow that keys are only usable
 *      on an specific boot stage.
 */
enum KeyPermissions {
    KEY_MANAGEMENT_KEY,
    ALLOW_EPHEMERAL_KEY_WRAPPING,
    ALLOW_HARDWARE_KEY_WRAPPING,
    ALLOW_PORTABLE_KEY_WRAPPING,
    BOOTSTATE_BINDING,
}
