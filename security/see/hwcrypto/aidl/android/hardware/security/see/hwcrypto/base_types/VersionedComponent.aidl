package android.hardware.security.see.hwcrypto.base_types;

/**
 * enum VersionedComponent - Enum describing the different components which version can be used for
 *                           version binding. Note that currently only SeeVersion is supported for
 *                           secure HAL operations. Other listed values represent components that
 *                           keymaster can bind versions to.
 *
 * @SeeVersion:
 *      Secure Excecution Environment version.
 * @BootPatchLevel:
 *      Boot partition version.
 * @VendorPatchLevel:
 *      Vendor partition version.
 * @OsVersion:
 *      System partition version.
 * @OsPatchLevel:
 *      Android patch version partition version, representing the year and month of the
 *      last update to the system.
 */
@Backing(type="byte")
enum VersionedComponent {
    SEE_VERSION = 0,
    BOOT_PATCH_LEVEL = 1,
    VENDOR_PATCH_LEVEL = 2,
    OS_VERSION = 3,
    OS_PATCH_LEVEL = 4,
}
