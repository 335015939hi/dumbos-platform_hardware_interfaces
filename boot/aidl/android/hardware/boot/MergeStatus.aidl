// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.boot;

@VintfStability
@Backing(type="int")
enum MergeStatus {
    /**
     * No snapshot or merge is in progress.
     */
    NONE = 0,
    /**
     * The merge status could not be determined.
     */
    UNKNOWN,
    /**
     * Partitions are being snapshotted, but no merge has been started.
     */
    SNAPSHOTTED,
    /**
     * At least one partition has merge is in progress.
     */
    MERGING,
    /**
     * A merge was in progress, but it was canceled by the bootloader.
     */
    CANCELLED,
}
