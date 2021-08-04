package android.hardware.tv.earc;

import android.hardware.tv.earc.EarcCapability;
import android.hardware.tv.earc.EarcControl;
import android.hardware.tv.earc.EarcEvent;
import android.hardware.tv.earc.EarcEventType;
import android.hardware.tv.earc.EarcStatus;
import android.hardware.tv.earc.Result;
import android.hardware.tv.earc.IHdmiEarcCallback;

/**
 * HDMI-EARC HAL interface definition.
 */
@VintfStability
interface IHdmiEarc {
    /**
     * Get the hardware variation of eARC support flag.
     *
     * This used by framework to consider the eARC feature in need.
     *
     * @return The eARC support status related with hardware variation.
     * 		   TRUE if support eARC.
     */
    boolean isSupport();

    /**
     * Get the current eARC port id.
     *
     * @return The current eARC port id.
     *         It shall start from "1" which indicates the "hdmi port 1".
     */
    int getPortId();

    /**
     * Get the current eARC status.
     *
     * Since eARC not rely on CEC message, Framework need to get the current eARC
     * hw status to avoid handle the ARC handshake at first.
     *
     * @param in portId The eARC port id from framework.
     *
     * @return The current eARC hw status as defined in EarcStatus.
     */
    EarcStatus getStatus(in int portId);

    /**
     * Get the capability of eARC RX device.
     *
     * The audio capability data structure as defined in hdmi2.1 spec 9.5 and example in
     * Appendix H, Which indicate the audio formats and sample rates that eARC device support.
     * Earc tx shall only send Basic audio or audio that capability indicates it supports.
     *
     * @param in portId The eARC port id from framework.
     *
     * @return The audio capability from device.
     */
    EarcCapability getCapability(in int portId);

    /**
     * Get the eARC latency value.
     *
     * The latency value from device used by audio framework to control/adjust the audio latency feature.
     *
     * @param in portId The eARC port id from framework.
     *
     * @return The latency value from device.
     */
    int getLatency(in int portId);

    /**
     * Control the eARC Audio latency.
     *
     * If support the eARC audio latency feature, Audio framework or others would adjust the latency value
     * according to the which from eARC device and send back.
     *
     * @param in latency The adjusted latency value.
     *
     * @return Result code of the operation. OK if successful, otherwise fail.
     */
    Result controlAudioLatency(in int latency);

    /**
     * Control the eARC feature.
     *
     * Users should control the eARC feature in user interface.
     *
     * @param in control The bool parameter for user to control the eARC feature.
     *
     * @return Result code of the operation. OK if successful, otherwise fail.
     */
    Result controlFeature(in EarcControl control);

    /**
     * Control ARC Enable.
     *
     * @param in enable The bool parameter for CEC HAL to control ARC Enable.
     *
     * @return Result code of the operation. OK if successful, otherwise fail.
     */
    Result enableArc(in boolean enable);

    /**
     * Set the eARC callback event.
     *
     * It's used by the framework to receive status change event, capability change event,
     * and latency change event. Only one callback client is supported.
     *
     * @param in callback Callback function to pass eARC event to the system.
     *
     * @return Result code of the operation. OK if successful, otherwise fail.
     */
    Result setCallback(in IHdmiEarcCallback callback);
}
