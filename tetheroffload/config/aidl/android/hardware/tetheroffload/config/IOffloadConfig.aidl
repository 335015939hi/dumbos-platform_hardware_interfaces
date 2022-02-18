// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.tetheroffload.config;

/**
 * Interface used for configuring the hardware management process
 */
@VintfStability
interface IOffloadConfig {
    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Provides bound netlink file descriptors for use in the management process
     *
     * @param fd1   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY).
     * @param fd2   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY).
     *
     * @param out success true if successful, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     */
    void setHandles(in android.os.NativeHandle fd1, in android.os.NativeHandle fd2,
            out boolean success, out String errMsg);
}
