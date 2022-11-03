// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.secure_element;

public class Translate {
    static public android.hardware.secure_element.LogicalChannelResponse h2aTranslate(
            android.hardware.secure_element.V1_0.LogicalChannelResponse in) {
        android.hardware.secure_element.LogicalChannelResponse out =
                new android.hardware.secure_element.LogicalChannelResponse();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.channelNumber > 127 || in.channelNumber < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.channelNumber");
        }
        out.channelNumber = in.channelNumber;
        if (in.selectResponse != null) {
            out.selectResponse = new byte[in.selectResponse.size()];
            for (int i = 0; i < in.selectResponse.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.selectResponse.get(i) > 127 || in.selectResponse.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.selectResponse.get(i)");
                }
                out.selectResponse[i] = in.selectResponse.get(i);
            }
        }
        return out;
    }
}