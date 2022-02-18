// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.tetheroffload.control;

import android.hardware.tetheroffload.control.IPv4AddrPortPair;
import android.hardware.tetheroffload.control.NetworkProtocol;

@VintfStability
parcelable NatTimeoutUpdate {
    IPv4AddrPortPair src;
    IPv4AddrPortPair dst;
    NetworkProtocol proto;
}
