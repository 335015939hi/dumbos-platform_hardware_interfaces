/**
 *
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.tetheroffload.control;

import android.hardware.tetheroffload.control.ITetheringOffloadCallback;

/**
 * Interface used to control the lifecycle of tethering offload. Note that callbacks of 1.1 HAL
 * can be registered with the existing callback registration methods from 1.0 HAL.
 */
// Interface inherits from android.hardware.tetheroffload.control@1.0::IOffloadControl but AIDL does not support interface inheritance (methods have been flattened).
@VintfStability
interface IOffloadControl {
    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Configure a downstream interface and prefix in the hardware management process that may be
     * forwarded.
     *
     * The prefix may be an IPv4 or an IPv6 address to signify which family can be offloaded from the
     * specified tether interface.  The list of IPv4 and IPv6 downstreams that are configured may
     * differ.
     *
     * If the given protocol, as determined by the prefix, has an upstream set,
     * the hardware may begin forwarding traffic between the upstream and any devices on the
     * downstream interface that have IP addresses within the specified prefix. Traffic from the same
     * downstream interfaces is unaffected and must be forwarded if and only if it was already
     * being forwarded.
     *
     * If no upstream is currently configured, then these downstream interface and prefixes must be
     * preserved so that offload may begin in the future when an upstream is set.
     *
     * This API does not replace any previously configured downstreams and must be explictly removed
     * by calling removeDownstream.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Tether interface
     * @param prefix Downstream prefix depicting addresses that may be offloaded.
     *               For e.g. 192.168.1.12/24 or 2001:4860:0684::/64)
     *
     * @param out success true if success, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     *
     * Remarks: The hardware management process may fail this call in a normal situation.  This can
     *          happen because the hardware cannot support the current number of prefixes, the
     *          hardware cannot support concurrent offload on multiple interfaces, the hardware
     *          cannot currently support offload on the tether interface for some reason, or any
     *          other dynamic configuration issues which may occur.  In this case,
     *          traffic must remain unaffected and must be forwarded if and only if it was already
     *          being forwarded.
     */
    void addDownstream(in String iface, in String prefix, out boolean success, out String errMsg);

    // FIXME: AIDL does not allow long to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow long to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Query offloaded traffic statistics forwarded to an upstream address.
     *
     * Return statistics that have transpired since the last query.  This would include
     * statistics from all offloaded downstream tether interfaces that have been forwarded to this
     * upstream interface.  After returning the statistics, the counters are reset to zero.
     *
     * Only offloaded statistics must be returned by this API, software stats must not be
     * returned.
     *
     * @param upstream Upstream interface on which traffic exited/entered
     *
     * @param out rxBytes values depicting the received bytes
     * @param out txBytes values depicting the transmitted bytes
     */
    void getForwardedStats(in String upstream, out long rxBytes, out long txBytes);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Indicates intent to start offload for tethering in immediate future.
     *
     * This API must be called exactly once the first time that Tethering is requested by
     * the user.
     *
     * If this API is called multiple times without first calling stopOffload, then the subsequent
     * calls must fail without changing the state of the server.
     *
     * If for some reason, the hardware is currently unable to support offload, this call must fail.
     *
     * @param cb Assuming success, this callback must provide unsolicited updates of offload status.
     *           It is assumed to be valid until stopOffload is called.
     *
     * @param out success true if initialization is successful, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     *
     * Remarks: Initializing offload does not imply that any upstreams or downstreams have yet been,
     * or even will be, chosen.  This API is symmetrical with stopOffload.
     */
    void initOffload(in ITetheringOffloadCallback cb, out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Remove a downstream prefix that may be forwarded from the hardware management process.
     *
     * The prefix may be an IPv4 or an IPv6 address. If it was not previously configured using
     * addDownstream, then this must be a no-op.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Tether interface
     * @param prefix Downstream prefix depicting address that must no longer be offloaded
     *               For e.g. 192.168.1.12/24 or 2001:4860:0684::/64)
     *
     * @param out success true if success, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     */
    void removeDownstream(in String iface, in String prefix,
        out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Instruct hardware to stop forwarding traffic and send a callback after limit bytes have been
     * transferred in either direction on this upstream interface.
     *
     * The limit must be applied to all traffic on the given upstream interface.  This
     * includes hardware forwarded traffic, software forwarded traffic, and AP-originated traffic.
     * IPv4 and IPv6 traffic both count towards the same limit.  IP headers are included in the
     * byte count limit, but, link-layer headers are not.
     *
     * This API may only be called while offload is occurring on this upstream.  The hardware
     * management process is not expected to cache the value and apply the quota once offload is
     * started.  This cache is not expected, because the limit value would likely become stale over
     * time and would not reflect any new traffic that has occurred.
     *
     * This limit must replace any previous limit.  It may be interpreted as "tell me when
     * <limit> bytes have been transferred (in either direction) on <upstream>, starting
     * now and counting from zero."
     *
     * Once the limit is reached, the callback registered in initOffload must be called to indicate
     * this event and all offload must be stopped.  If offload is desired again, the hardware
     * management process must be completely reprogrammed by calling setUpstreamParameters and
     * addDownstream again.  Note that it is not necessary to call initOffload again to resume offload
     * if stopOffload was not called by the client.
     *
     * @param upstream Upstream interface name that limit must apply to
     * @param limit    Bytes limit that can occur before action must be taken
     *
     * @param out success true if limit is applied, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     */
    void setDataLimit(in String upstream, in long limit, out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Instruct hardware to send callbacks after certain number of bytes have been transferred in
     * either direction on this upstream interface.
     *
     * The specified quota bytes must be applied to all traffic on the given upstream interface.
     * This includes hardware forwarded traffic, software forwarded traffic, and AP-originated
     * traffic. IPv4 and IPv6 traffic both count towards the same quota. IP headers are included
     * in the byte count quota, but, link-layer headers are not.
     *
     * This API may only be called while offload is occurring on this upstream. The hardware
     * management process MUST NOT store the values when offload is not started and applies once
     * offload is started. This is because the quota values would likely become stale over
     * time and would not reflect any new traffic that has occurred.
     *
     * This API replaces {@link @1.0::IOffloadControl::setDataLimit}, the framework will always
     * call setDataWarningAndLimit on 1.1 implementations, and setDataLimit on 1.0 implementations.
     * Thus, no interaction between the two APIs need to be addressed.
     *
     * The specified quota bytes MUST replace any previous quotas set by
     * {@code setDataWarningAndLimit} specified on the same interface. It may be interpreted as
     * "tell me when either <warningBytes> or <limitBytes> bytes have been transferred
     * (in either direction), and stop offload when <limitBytes> bytes have been transferred,
     * starting now and counting from zero on <upstream>."
     *
     * Once the {@code warningBytes} is reached, the callback registered in initOffload must be
     * called with {@code OFFLOAD_WARNING_REACHED} to indicate this event. Once the event fires
     * for this upstream, no further {@code OFFLOAD_WARNING_REACHED} event will be fired for this
     * upstream unless this method is called again with the same interface. Note that there is
     * no need to call initOffload again to resume offload if stopOffload was not called by the
     * client.
     *
     * Similarly, Once the {@code limitBytes} is reached, the callback registered in initOffload
     * must be called with {@code OFFLOAD_STOPPED_LIMIT_REACHED} to indicate this event. Once
     * the event fires for this upstream, no further {@code OFFLOAD_STOPPED_LIMIT_REACHED}
     * event will be fired for this upstream unless this method is called again with the same
     * interface. However, unlike {@code warningBytes}, when {@code limitBytes} is reached,
     * all offload must be stopped. If offload is desired again, the hardware management
     * process must be completely reprogrammed by calling setUpstreamParameters and
     * addDownstream again.
     *
     * Note that when one of the quota bytes is reached, the other one is still considered valid
     * unless this method is called again with the same interface.
     *
     * @param upstream Upstream interface name that quota must apply to.
     * @param warningBytes The quota of warning, defined as the number of bytes, starting from
     *                     zero and counting from now.
     * @param limitBytes The quota of limit, defined as the number of bytes, starting from zero
     *                   and counting from now.
     *
     * @param out success true if quota is applied, false otherwise
     * @param out errMsg a human readable string if error has occurred.
     */
    void setDataWarningAndLimit(in String upstream, in long warningBytes, in long limitBytes,
        out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Instruct management process not to forward traffic destined to or from the specified prefixes.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param prefixes List containing fully specified prefixes. For e.g. 192.168.1.12/24
     * or 2001:4860:0684:0:0:0:0:0:1002/64
     *
     * @param out success true if success, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     *
     * Remarks: This list overrides any previously specified list
     */
    void setLocalPrefixes(in String[] prefixes, out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Instruct hardware to start forwarding traffic to the specified upstream.
     *
     * When iface, v4Addr, and v4Gw are all non-null, the management process may begin forwarding
     * any currently configured or future configured IPv4 downstreams to this upstream interface.
     *
     * If any of the previously three mentioned parameters are null, then any current IPv4 offload
     * must be stopped.
     *
     * When iface and v6Gws are both non-null, and in the case of v6Gws, are not empty, the
     * management process may begin forwarding any currently configured or future configured IPv6
     * downstreams to this upstream interface.
     *
     * If either of the two above parameters are null, or no V6 Gateways are provided, then IPv6
     * offload must be stopped.
     *
     * This API may only be called after initOffload and before stopOffload.
     *
     * @param iface  Upstream interface name.  Note that only one is needed because IPv4 and IPv6
     *               interfaces cannot be different (only known that this can occur during software
     *               xlat, which cannot be offloaded through hardware anyways).  If the iface is
     *               null, offload must be stopped.
     * @param v4Addr The local IPv4 address assigned to the provided upstream interface, i.e. the
     *               IPv4 address the packets are NATed to. For e.g. 192.168.1.12.
     * @param v4Gw   The IPv4 address of the IPv4 gateway on the upstream interface.
     *               For e.g. 192.168.1.1
     * @param v6Gws  A list of IPv6 addresses (for e.g. 2001:4860:0684:0:0:0:0:0:1002) for possible
     *               IPv6 gateways on the upstream interface.
     *
     * @param out success true if success, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     *
     * Remarks: This overrides any previously configured parameters.
     */
    void setUpstreamParameters(in String iface, in String v4Addr, in String v4Gw,
        in String[] v6Gws, out boolean success, out String errMsg);

    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    /**
     * Indicate desire to tear down all tethering offload.
     *
     * Called after tethering is no longer requested by the user. Any remaining offload must
     * be subsequently torn down by the management process.  Upon success, the callback registered
     * in initOffload must be released, and offload must be stopped.
     *
     * @param out success true if offload is stopped, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     *
     * Remarks: Statistics must be reset by this API.
     */
    void stopOffload(out boolean success, out String errMsg);
}
