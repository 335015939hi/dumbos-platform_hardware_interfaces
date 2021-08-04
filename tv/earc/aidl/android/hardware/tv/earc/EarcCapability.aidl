package android.hardware.tv.earc;

@VintfStability
parcelable EarcCapability {
	/**
	 * HDMI EARC Capability. The max size
	 * defined in the Chapter 9.5.3.6 of HDMI2.1 spec.
	 */
	const int CAP_MAX_SIZE = 1 << 8;

    int len;

    byte[] payload;
}

