// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.boot;

/**
 * A result encapsulating whether a function returned true, false or
 * failed due to an invalid slot number
 */
@VintfStability
@Backing(type="int")
enum BoolResult {
    FALSE = 0,
    TRUE = 1,
    INVALID_SLOT = -1,
}
