package android.hardware.tv.earc;

import android.hardware.tv.earc.EarcEvent;

/**
 * The callback function that must be called by HAL implementation to notify
 * the system of those status change.
 ** Earc Status Change
 ** Earc Capability Change
 ** Earc Audio Latency Change
 */
@VintfStability
interface IHdmiEarcCallback {
	oneway void notify(in EarcEvent event);
}
