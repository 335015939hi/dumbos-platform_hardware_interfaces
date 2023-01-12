// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.boot;

/**
 * A command result encapsulating whether the command succeeded and
 * an error string.
 */
@VintfStability
parcelable CommandResult {
    boolean success;
    String errMsg;
}
