// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.tetheroffload.control;

public class Translate {
    static public android.hardware.tetheroffload.control.IPv4AddrPortPair h2aTranslate(
            android.hardware.tetheroffload.control.V1_0.IPv4AddrPortPair in) {
        android.hardware.tetheroffload.control.IPv4AddrPortPair out =
                new android.hardware.tetheroffload.control.IPv4AddrPortPair();
        out.addr = in.addr;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.port < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.port");
        }
        out.port = (char) in.port;
        return out;
    }

    static public android.hardware.tetheroffload.control.NatTimeoutUpdate h2aTranslate(
            android.hardware.tetheroffload.control.V1_0.NatTimeoutUpdate in) {
        android.hardware.tetheroffload.control.NatTimeoutUpdate out =
                new android.hardware.tetheroffload.control.NatTimeoutUpdate();
        out.src = h2aTranslate(in.src);
        out.dst = h2aTranslate(in.dst);
        out.proto = in.proto;
        return out;
    }
}