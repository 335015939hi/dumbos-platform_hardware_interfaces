package android.hardware.tv.earc;

import android.hardware.tv.earc.EarcEventType;
import android.hardware.tv.earc.EarcStatus;

/**
 * HDMI EARC Status Change Event.
 */
@VintfStability
parcelable EarcEvent {
	EarcEventType type;
	EarcStatus status;
}

